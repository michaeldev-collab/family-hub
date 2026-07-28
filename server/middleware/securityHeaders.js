'use strict';

/**
 * Minimal security headers for the same-origin static admin UI + JSON API.
 * When Clerk is enabled, allow Clerk fronts + Cloudflare Turnstile CAPTCHA.
 * @see https://clerk.com/docs/guides/secure/best-practices/csp-headers
 */
const config = require('../config/env');

function clerkFrontendApiHost(publishableKey) {
  if (!publishableKey) return '';
  try {
    const payload = publishableKey.replace(/^pk_(test|live)_/, '');
    const decoded = Buffer.from(payload, 'base64').toString('utf8');
    const host = decoded.split('$')[0];
    return host && host.includes('.') ? host : '';
  } catch (_) {
    return '';
  }
}

function securityHeaders(req, res, next) {
  res.setHeader('X-Content-Type-Options', 'nosniff');
  res.setHeader('X-Frame-Options', 'DENY');
  res.setHeader('Referrer-Policy', 'no-referrer');
  res.setHeader(
    'Permissions-Policy',
    'camera=(), microphone=(), geolocation=()'
  );

  const scriptSrc = ["'self'", "'unsafe-inline'"];
  const connectSrc = ["'self'"];
  const frameSrc = ["'none'"];
  const imgSrc = ["'self'", 'data:'];
  const workerSrc = ["'self'", 'blob:'];
  const styleSrc = ["'self'", "'unsafe-inline'", 'https://fonts.googleapis.com'];
  const fontSrc = ["'self'", 'https://fonts.gstatic.com'];
  const formAction = ["'self'"];

  if (config.clerkEnabled) {
    const fapi = clerkFrontendApiHost(config.clerkPublishableKey);
    const clerkHosts = [
      'https://*.clerk.accounts.dev',
      'https://*.clerk.com',
      'https://cdn.jsdelivr.net',
      'https://challenges.cloudflare.com',
      'https://*.protect.clerk.com',
    ];
    if (fapi) {
      clerkHosts.push(`https://${fapi}`);
      // Parent app domain (e.g. butler.3d-design-labs.com) for satellite / redirects.
      const parts = fapi.split('.');
      if (parts[0] === 'clerk' && parts.length > 2) {
        clerkHosts.push(`https://${parts.slice(1).join('.')}`);
      }
    }

    scriptSrc.push(...clerkHosts);
    connectSrc.push(
      ...clerkHosts,
      'https://api.clerk.com',
      'https://clerk-telemetry.com',
      'https://*.clerk-telemetry.com'
    );
    frameSrc.length = 0;
    frameSrc.push("'self'", ...clerkHosts);
    imgSrc.push('https://img.clerk.com', 'https://*.clerk.com', 'https://challenges.cloudflare.com');
    styleSrc.push('https://*.clerk.accounts.dev', 'https://*.clerk.com');
    fontSrc.push('https://*.clerk.accounts.dev', 'https://*.clerk.com');
    if (fapi) {
      styleSrc.push(`https://${fapi}`);
      fontSrc.push(`https://${fapi}`);
    }
    formAction.push(
      'https://*.clerk.accounts.dev',
      'https://*.clerk.com',
      'https://accounts.google.com',
      'https://challenges.cloudflare.com'
    );
    if (fapi) formAction.push(`https://${fapi}`);
  }

  res.setHeader(
    'Content-Security-Policy',
    [
      "default-src 'self'",
      `script-src ${scriptSrc.join(' ')}`,
      `style-src ${styleSrc.join(' ')}`,
      `font-src ${fontSrc.join(' ')}`,
      `img-src ${imgSrc.join(' ')}`,
      `connect-src ${connectSrc.join(' ')}`,
      `frame-src ${frameSrc.join(' ')}`,
      `worker-src ${workerSrc.join(' ')}`,
      "frame-ancestors 'none'",
      "base-uri 'self'",
      `form-action ${formAction.join(' ')}`,
    ].join('; ')
  );
  next();
}

module.exports = { securityHeaders };
