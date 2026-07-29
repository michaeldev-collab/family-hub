'use strict';

const config = require('../config/env');

function tokenFromRequest(req) {
  return req.get('x-family-hub-token') || '';
}

function isCloudflareProxied(req) {
  return Boolean(req.get('cf-ray') || req.get('cf-connecting-ip'));
}

/** Block ESP32 panel credentials when the request arrived via Cloudflare Tunnel. */
function rejectPanelViaTunnel(req, res, next) {
  if (!config.rejectPanelViaTunnel) return next();
  const token = tokenFromRequest(req);
  if (!token || !config.panelToken || token !== config.panelToken) return next();
  if (!isCloudflareProxied(req)) return next();
  return res.status(403).json({
    error: 'panel-lan-only',
    message: 'Panel token must be used on the LAN, not the public tunnel',
  });
}

function parseAllowedUserIds(raw) {
  if (!raw) return null;
  const ids = String(raw)
    .split(',')
    .map((s) => s.trim())
    .filter(Boolean);
  return ids.length ? new Set(ids) : null;
}

function roleFromClerkClaims(payload) {
  const meta = payload.publicMetadata || payload.public_metadata || {};
  const role = String(meta.role || 'parent').toLowerCase();
  return role === 'kid' ? 'kid' : 'parent';
}

/**
 * Test bypass: Authorization Bearer test.<base64url(JSON)>
 * JSON shape: { sub, publicMetadata?: { role } }
 * Only when CLERK_TEST_BYPASS=true (never in production).
 */
function verifyTestBearer(token) {
  if (!config.clerkTestBypass || !token.startsWith('test.')) return null;
  try {
    const json = Buffer.from(token.slice(5), 'base64url').toString('utf8');
    const payload = JSON.parse(json);
    if (!payload || !payload.sub) return null;
    return payload;
  } catch {
    return null;
  }
}

async function verifyClerkToken(token) {
  const testPayload = verifyTestBearer(token);
  if (testPayload) return testPayload;

  if (!config.clerkSecretKey) {
    const err = new Error('clerk-not-configured');
    err.code = 'clerk-not-configured';
    throw err;
  }

  const { verifyToken } = require('@clerk/backend');
  const options = { secretKey: config.clerkSecretKey };
  if (config.clerkAuthorizedParties.length) {
    options.authorizedParties = config.clerkAuthorizedParties;
  }
  return verifyToken(token, options);
}

function bearerToken(req) {
  const header = req.get('authorization') || '';
  const m = header.match(/^Bearer\s+(.+)$/i);
  return m ? m[1].trim() : '';
}

async function attachClerkAuth(req) {
  // Prefer auth already resolved by @clerk/express clerkMiddleware().
  try {
    const { getAuth } = require('@clerk/express');
    const authState = getAuth(req);
    if (authState && authState.isAuthenticated && authState.userId) {
      const claims = authState.sessionClaims || {};
      const meta =
        claims.publicMetadata ||
        claims.public_metadata ||
        (authState.sessionClaims && authState.sessionClaims.metadata) ||
        {};
      const role = roleFromClerkClaims({ publicMetadata: meta });
      const allowed = parseAllowedUserIds(config.clerkAllowedUserIds);
      if (allowed && !allowed.has(authState.userId)) {
        const err = new Error('User is not allowlisted for this household');
        err.status = 403;
        err.code = 'forbidden';
        throw err;
      }
      req.auth = {
        type: 'clerk',
        userId: authState.userId,
        role,
        sessionId: authState.sessionId || null,
      };
      return true;
    }
  } catch (err) {
    if (err.status === 403) throw err;
    // getAuth unavailable or unauthenticated — fall through to Bearer verify
  }

  const token = bearerToken(req);
  if (!token) return false;
  const payload = await verifyClerkToken(token);
  const userId = payload.sub || payload.userId;
  if (!userId) return false;

  const allowed = parseAllowedUserIds(config.clerkAllowedUserIds);
  if (allowed && !allowed.has(userId)) {
    const err = new Error('User is not allowlisted for this household');
    err.status = 403;
    err.code = 'forbidden';
    throw err;
  }

  req.auth = {
    type: 'clerk',
    userId,
    role: roleFromClerkClaims(payload),
    sessionId: payload.sid || null,
  };
  return true;
}

/**
 * Write auth for browser CRUD.
 * - Clerk configured: require Clerk JWT (or full WRITE_TOKEN).
 * - Else: optional WRITE_TOKEN / LAN trust (legacy).
 * Panel-only PANEL_TOKEN does not pass here.
 */
async function optionalWriteAuth(req, res, next) {
  try {
    const token = tokenFromRequest(req);
    if (config.writeToken && token === config.writeToken) {
      req.auth = { type: 'write-token', role: 'parent' };
      return next();
    }

    if (config.clerkEnabled) {
      const ok = await attachClerkAuth(req);
      if (!ok) {
        return res.status(401).json({
          error: 'unauthorized',
          message: 'Sign in required',
        });
      }
      return next();
    }

    if (!config.writeToken) return next();
    return res.status(401).json({ error: 'unauthorized', message: 'Invalid write token' });
  } catch (err) {
    if (err.status === 403) {
      return res.status(403).json({ error: err.code || 'forbidden', message: err.message });
    }
    return res.status(401).json({
      error: 'unauthorized',
      message: err.message || 'Invalid credentials',
    });
  }
}

/**
 * Panel chore/grocery toggle auth: WRITE_TOKEN, PANEL_TOKEN, Clerk, or LAN trust.
 */
async function panelCompleteAuth(req, res, next) {
  try {
    const token = tokenFromRequest(req);
    const { writeToken, panelToken } = config;

    if (token && writeToken && token === writeToken) {
      req.auth = { type: 'write-token', role: 'parent' };
      return next();
    }
    if (token && panelToken && token === panelToken) {
      req.auth = { type: 'panel', role: 'parent' };
      return next();
    }

    if (config.clerkEnabled) {
      const ok = await attachClerkAuth(req);
      if (ok) return next();
      if (writeToken || panelToken) {
        return res.status(401).json({ error: 'unauthorized', message: 'Invalid panel token' });
      }
      return res.status(401).json({ error: 'unauthorized', message: 'Sign in required' });
    }

    if (!writeToken && !panelToken) return next();
    return res.status(401).json({ error: 'unauthorized', message: 'Invalid panel token' });
  } catch (err) {
    if (err.status === 403) {
      return res.status(403).json({ error: err.code || 'forbidden', message: err.message });
    }
    return res.status(401).json({
      error: 'unauthorized',
      message: err.message || 'Invalid credentials',
    });
  }
}

/** Require authenticated Clerk user (no LAN trust / panel token). */
async function requireClerk(req, res, next) {
  if (!config.clerkEnabled) {
    return res.status(503).json({ error: 'clerk-disabled', message: 'Clerk is not configured' });
  }
  try {
    const ok = await attachClerkAuth(req);
    if (!ok) {
      return res.status(401).json({ error: 'unauthorized', message: 'Sign in required' });
    }
    return next();
  } catch (err) {
    if (err.status === 403) {
      return res.status(403).json({ error: err.code || 'forbidden', message: err.message });
    }
    return res.status(401).json({
      error: 'unauthorized',
      message: err.message || 'Invalid credentials',
    });
  }
}

function requireRole(role) {
  const needed = String(role || 'parent').toLowerCase();
  return function roleGuard(req, res, next) {
    if (req.auth?.type === 'write-token') return next();
    if (req.auth?.type === 'panel') {
      return res.status(403).json({
        error: 'forbidden',
        message: 'Panel token cannot perform this action',
      });
    }
    if (!config.clerkEnabled) return next();
    const have = req.auth?.role || 'kid';
    if (needed === 'parent' && have !== 'parent') {
      return res.status(403).json({
        error: 'forbidden',
        message: 'Parent role required',
      });
    }
    return next();
  };
}

const requireParent = requireRole('parent');

module.exports = {
  optionalWriteAuth,
  panelCompleteAuth,
  requireClerk,
  requireRole,
  requireParent,
  rejectPanelViaTunnel,
  attachClerkAuth,
  verifyClerkToken,
  bearerToken,
};
