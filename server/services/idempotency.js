'use strict';

const { getDb } = require('../db/connection');

const IDEMPOTENCY_TTL_DAYS = 30;

function pruneOldKeys(maxAgeDays = IDEMPOTENCY_TTL_DAYS) {
  const db = getDb();
  db.prepare(
    `DELETE FROM idempotency_keys
     WHERE created_at < datetime('now', '-' || ? || ' days')`
  ).run(maxAgeDays);
}

function getCachedResponse(eventId) {
  const db = getDb();
  const row = db
    .prepare('SELECT response_json FROM idempotency_keys WHERE event_id = ?')
    .get(eventId);
  if (!row) return null;
  return JSON.parse(row.response_json);
}

function storeCachedResponse(eventId, response) {
  const db = getDb();
  pruneOldKeys();
  db.prepare(
    `INSERT OR REPLACE INTO idempotency_keys (event_id, response_json)
     VALUES (?, ?)`
  ).run(eventId, JSON.stringify(response));
}

function logWrite(action, entity, entityId, source = 'api') {
  const db = getDb();
  db.prepare(
    `INSERT INTO write_log (action, entity, entity_id, source)
     VALUES (?, ?, ?, ?)`
  ).run(action, entity, entityId || null, source);
}

module.exports = {
  getCachedResponse,
  storeCachedResponse,
  logWrite,
  pruneOldKeys,
  IDEMPOTENCY_TTL_DAYS,
};
