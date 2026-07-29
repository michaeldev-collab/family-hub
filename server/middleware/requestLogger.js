'use strict';

function redactUrl(url) {
  if (!url || typeof url !== 'string') return url;
  return url
    .replace(/([?&](?:token|access_token|write_token|panel_token)=)[^&]*/gi, '$1[redacted]')
    .replace(/([?&](?:authorization)=)[^&]*/gi, '$1[redacted]');
}

function requestLogger(req, res, next) {
  const start = Date.now();
  res.on('finish', () => {
    if (req.method === 'GET' && req.path === '/api/health') return;
    const ms = Date.now() - start;
    const pathForLog = redactUrl(req.originalUrl);
    const line = `[${new Date().toISOString()}] ${req.method} ${pathForLog} ${res.statusCode} ${ms}ms`;
    if (req.method !== 'GET' || res.statusCode >= 400) {
      console.log(line);
    }
  });
  next();
}

module.exports = requestLogger;
module.exports.redactUrl = redactUrl;
