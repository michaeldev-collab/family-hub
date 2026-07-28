'use strict';

const path = require('path');
const express = require('express');
const config = require('./config/env');
const { initDb } = require('./db/init');
const { closeDb } = require('./db/connection');
const requestLogger = require('./middleware/requestLogger');
const { securityHeaders } = require('./middleware/securityHeaders');

const healthRoutes = require('./routes/health.routes');
const dashboardRoutes = require('./routes/dashboard.routes');
const authRoutes = require('./routes/auth.routes');
const groceryRoutes = require('./routes/grocery.routes');
const choresRoutes = require('./routes/chores.routes');
const dinnerRoutes = require('./routes/dinner.routes');
const notesRoutes = require('./routes/notes.routes');
const membersRoutes = require('./routes/members.routes');
const eventsRoutes = require('./routes/events.routes');
const remindersRoutes = require('./routes/reminders.routes');
const { DomainError } = require('./services/domainCore');
const { rejectPanelViaTunnel } = require('./middleware/auth');

function createApp() {
  initDb();

  const app = express();
  app.disable('x-powered-by');
  app.set('trust proxy', config.trustProxy);

  app.use(securityHeaders);
  app.use(requestLogger);
  app.use(express.json({ limit: '32kb' }));
  app.use(express.urlencoded({ extended: false, limit: '32kb' }));

  // Official Clerk Express middleware — attaches auth from session cookie/JWT.
  // Skip under CLERK_TEST_BYPASS (unit tests use synthetic Bearer tokens).
  if (config.clerkEnabled && !config.clerkTestBypass) {
    const { clerkMiddleware } = require('@clerk/express');
    app.use(clerkMiddleware());
  }

  app.use('/api', rejectPanelViaTunnel);

  app.use(
    express.static(path.join(__dirname, '..', 'web'), {
      fallthrough: true,
      index: 'index.html',
    })
  );

  app.use('/api', healthRoutes);
  app.use('/api', dashboardRoutes);
  app.use('/api/auth', authRoutes);
  app.use('/api/grocery', groceryRoutes);
  app.use('/api/chores', choresRoutes);
  app.use('/api/dinner', dinnerRoutes);
  app.use('/api/notes', notesRoutes);
  app.use('/api/members', membersRoutes);
  app.use('/api/events', eventsRoutes);
  app.use('/api/reminders', remindersRoutes);

  app.use((req, res) => {
    if (req.originalUrl.startsWith('/api')) {
      return res.status(404).json({ error: 'not-found' });
    }
    return res.status(404).send('Not found');
  });

  app.use((err, req, res, next) => {
    if (err instanceof DomainError) {
      return res.status(err.status).json({
        error: err.code,
        ...(err.details === undefined ? {} : { details: err.details }),
      });
    }
    if (err && (err.type === 'entity.too.large' || err.status === 413)) {
      return res.status(413).json({ error: 'request-too-large' });
    }
    if (err instanceof SyntaxError && err.type === 'entity.parse.failed') {
      return res.status(400).json({ error: 'invalid-json' });
    }
    if (err && err.code === 'ERR_SQLITE_ERROR') {
      const message = String(err.message || '');
      const conflict = /UNIQUE constraint failed/.test(message);
      return res.status(conflict ? 409 : 422).json({
        error: conflict ? 'resource-conflict' : 'constraint-violation',
      });
    }
    console.error('[error]', err && err.code ? err.code : 'internal-error');
    if (req.originalUrl.startsWith('/api')) {
      return res.status(500).json({ error: 'internal-error' });
    }
    return res.status(500).send('Internal error');
  });

  return app;
}

function start(options = {}) {
  const port = options.port !== undefined ? options.port : config.port;
  const host = options.host !== undefined ? options.host : config.host;
  const app = createApp();
  const server = app.listen(port, host, () => {
    console.log(
      `[family-hub] v${config.version} listening on http://${host}:${port} (${config.nodeEnv})`
    );
  });
  const shutdown = () => {
    server.close(() => {
      closeDb();
      process.exit(0);
    });
  };
  process.on('SIGINT', shutdown);
  process.on('SIGTERM', shutdown);
  return { app, server };
}

if (require.main === module) {
  start();
}

module.exports = { createApp, start };
