'use strict';

function requestLogger(req, res, next) {
  const start = Date.now();
  res.on('finish', () => {
    if (req.method === 'GET' && req.path === '/api/health') return;
    const ms = Date.now() - start;
    const line = `[${new Date().toISOString()}] ${req.method} ${req.originalUrl} ${res.statusCode} ${ms}ms`;
    if (req.method !== 'GET' || res.statusCode >= 400) {
      console.log(line);
    }
  });
  next();
}

module.exports = requestLogger;
