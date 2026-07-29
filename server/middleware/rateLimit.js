'use strict';

/**
 * Lightweight in-memory rate limit for API writes.
 * GETs (panel poll) are not limited. Tuned for a single household.
 */

function clientKey(req) {
  const ip = req.ip || req.socket?.remoteAddress || 'unknown';
  return String(ip);
}

function createWriteRateLimit(options = {}) {
  const windowMs = Number(options.windowMs) > 0 ? Number(options.windowMs) : 60_000;
  const max = Number(options.max) > 0 ? Number(options.max) : 180;
  const buckets = new Map();

  function prune(now) {
    if (buckets.size < 500) return;
    for (const [key, bucket] of buckets) {
      if (now - bucket.start >= windowMs) buckets.delete(key);
    }
  }

  function writeRateLimit(req, res, next) {
    if (req.method === 'GET' || req.method === 'HEAD' || req.method === 'OPTIONS') {
      return next();
    }
    if (!req.originalUrl.startsWith('/api')) return next();

    const now = Date.now();
    prune(now);
    const key = clientKey(req);
    let bucket = buckets.get(key);
    if (!bucket || now - bucket.start >= windowMs) {
      bucket = { start: now, count: 0 };
      buckets.set(key, bucket);
    }
    bucket.count += 1;
    if (bucket.count > max) {
      res.setHeader('Retry-After', String(Math.ceil(windowMs / 1000)));
      return res.status(429).json({
        error: 'rate-limited',
        message: 'Too many write requests; try again shortly',
      });
    }
    return next();
  }

  writeRateLimit._buckets = buckets;
  writeRateLimit._reset = () => buckets.clear();
  return writeRateLimit;
}

module.exports = { createWriteRateLimit };
