'use strict';

const { getDb } = require('../db/connection');

const STATE_VERSION_KEY = 'state_version';

function getStateVersion(db = getDb()) {
  const row = db
    .prepare('SELECT value FROM settings WHERE key = ?')
    .get(STATE_VERSION_KEY);
  return row ? Number.parseInt(row.value, 10) || 0 : 0;
}

function bumpStateVersion(db = getDb()) {
  const next = getStateVersion(db) + 1;
  db.prepare(
    `INSERT INTO settings (key, value) VALUES (?, ?)
     ON CONFLICT(key) DO UPDATE SET value = excluded.value`
  ).run(STATE_VERSION_KEY, String(next));
  return next;
}

function ensureStateVersion(db = getDb()) {
  const row = db
    .prepare('SELECT value FROM settings WHERE key = ?')
    .get(STATE_VERSION_KEY);
  if (!row) {
    db.prepare('INSERT INTO settings (key, value) VALUES (?, ?)').run(
      STATE_VERSION_KEY,
      '0'
    );
  }
}

module.exports = { getStateVersion, bumpStateVersion, ensureStateVersion };
