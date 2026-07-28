'use strict';

const { getDb } = require('../db/connection');
const { newId, clampText, rowToBool } = require('../utils/helpers');
const { bumpStateVersion, getStateVersion } = require('./stateVersion');
const { getCachedResponse, storeCachedResponse } = require('./idempotency');

const MAX_IDEMPOTENCY_KEY_LEN = 128;

function choreCompleteCacheKey(choreId, idempotencyKey) {
  return `chore-complete:${choreId}:${idempotencyKey}`;
}

function normalizeIdempotencyKey(raw) {
  if (raw === undefined || raw === null || raw === '') return null;
  if (typeof raw !== 'string') return null;
  const key = raw.trim();
  if (!key || key.length > MAX_IDEMPOTENCY_KEY_LEN) return null;
  return key;
}

function mapRow(row) {
  if (!row) return null;
  return {
    id: row.id,
    title: row.title,
    assigneeId: row.assignee_id,
    dueDate: row.due_date,
    recurrence: row.recurrence,
    completed: rowToBool(row, 'completed'),
    completedAt: row.completed_at,
    createdAt: row.created_at,
    updatedAt: row.updated_at,
  };
}

function listChores({ includeCompleted = true } = {}) {
  const db = getDb();
  const sql = includeCompleted
    ? `SELECT * FROM chores ORDER BY completed ASC, due_date ASC, created_at DESC`
    : `SELECT * FROM chores WHERE completed = 0 ORDER BY due_date ASC, created_at DESC`;
  return db.prepare(sql).all().map(mapRow);
}

function getChore(id) {
  const db = getDb();
  return mapRow(db.prepare('SELECT * FROM chores WHERE id = ?').get(id));
}

function createChore({ title, assigneeId, dueDate, recurrence }) {
  const id = newId();
  const safeTitle = clampText(title, 200);
  if (!safeTitle) throw new Error('title-required');
  const db = getDb();
  db.prepare(
    `INSERT INTO chores (id, title, assignee_id, due_date, recurrence)
     VALUES (?, ?, ?, ?, ?)`
  ).run(id, safeTitle, assigneeId || null, dueDate || null, recurrence || null);
  bumpStateVersion(db);
  return getChore(id);
}

function updateChore(id, patch) {
  const existing = getChore(id);
  if (!existing) return null;
  const db = getDb();
  const completed = patch.completed !== undefined ? (patch.completed ? 1 : 0) : (existing.completed ? 1 : 0);
  const completedAt =
    patch.completed === true
      ? new Date().toISOString()
      : patch.completed === false
        ? null
        : existing.completedAt;
  db.prepare(
    `UPDATE chores
     SET title = ?, assignee_id = ?, due_date = ?, recurrence = ?,
         completed = ?, completed_at = ?, updated_at = datetime('now')
     WHERE id = ?`
  ).run(
    patch.title !== undefined ? clampText(patch.title, 200) : existing.title,
    patch.assigneeId !== undefined ? patch.assigneeId : existing.assigneeId,
    patch.dueDate !== undefined ? patch.dueDate : existing.dueDate,
    patch.recurrence !== undefined ? patch.recurrence : existing.recurrence,
    completed,
    completedAt,
    id
  );
  bumpStateVersion(db);
  return getChore(id);
}

function deleteChore(id) {
  const db = getDb();
  const result = db.prepare('DELETE FROM chores WHERE id = ?').run(id);
  if (result.changes > 0) bumpStateVersion(db);
  return result.changes > 0;
}

// Narrow, panel-facing completion. Unlike updateChore(), this touches ONLY the
// completion columns — it cannot change title, assignee, due date, recurrence,
// or reopen a chore — so it is safe to expose to the scoped panel credential.
//
// Body (optional):
//   idempotency_key — retry-safe; duplicate posts return the cached response
//   expected_state_version — if set and stale, status 'version-conflict' (HTTP 409)
//
// Returns status: 'not-found' | 'already-complete' | 'completed' |
//   'version-conflict' | 'cached' | 'invalid-idempotency-key'
function completeChore(id, body = {}) {
  const rawKey = body && body.idempotency_key;
  let idempotencyKey = null;
  if (rawKey !== undefined && rawKey !== null && rawKey !== '') {
    idempotencyKey = normalizeIdempotencyKey(rawKey);
    if (!idempotencyKey) {
      return { status: 'invalid-idempotency-key' };
    }
    const cached = getCachedResponse(choreCompleteCacheKey(id, idempotencyKey));
    if (cached) {
      return { status: 'cached', response: cached };
    }
  }

  const existing = getChore(id);
  if (!existing) return { status: 'not-found' };

  if (body && body.expected_state_version !== undefined) {
    const expected = Number(body.expected_state_version);
    const current = getStateVersion();
    if (!Number.isInteger(expected) || expected !== current) {
      return {
        status: 'version-conflict',
        state_version: current,
      };
    }
  }

  if (existing.completed) {
    const response = {
      ok: true,
      alreadyComplete: true,
      item: existing,
      state_version: getStateVersion(),
    };
    if (idempotencyKey) {
      storeCachedResponse(choreCompleteCacheKey(id, idempotencyKey), response);
    }
    return { status: 'already-complete', item: existing, response };
  }

  const db = getDb();
  db.prepare(
    `UPDATE chores
     SET completed = 1, completed_at = datetime('now'), updated_at = datetime('now')
     WHERE id = ?`
  ).run(id);
  bumpStateVersion(db);
  const item = getChore(id);
  const response = {
    ok: true,
    item,
    state_version: getStateVersion(db),
  };
  if (idempotencyKey) {
    storeCachedResponse(choreCompleteCacheKey(id, idempotencyKey), response);
  }
  return { status: 'completed', item, response };
}

module.exports = {
  listChores,
  getChore,
  createChore,
  updateChore,
  deleteChore,
  completeChore,
  MAX_IDEMPOTENCY_KEY_LEN,
};
