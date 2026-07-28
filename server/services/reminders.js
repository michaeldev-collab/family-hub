'use strict';

const { getDb } = require('../db/connection');
const { newId, clampText } = require('../utils/helpers');

const ENTITY_TYPES = new Set(['grocery', 'chore', 'dinner', 'note']);

function mapRow(row) {
  if (!row) return null;
  let notifyMemberIds = [];
  try {
    notifyMemberIds = JSON.parse(row.notify_member_ids || '[]');
  } catch (_) {
    notifyMemberIds = [];
  }
  return {
    id: row.id,
    entityType: row.entity_type,
    entityId: row.entity_id,
    remindAt: row.remind_at,
    message: row.message,
    notifyMemberIds,
    channel: row.channel,
    status: row.status,
    createdAt: row.created_at,
    updatedAt: row.updated_at,
  };
}

function listReminders({ entityType, entityId, status } = {}) {
  const db = getDb();
  const clauses = [];
  const params = [];

  if (entityType) {
    clauses.push('entity_type = ?');
    params.push(entityType);
  }
  if (entityId) {
    clauses.push('entity_id = ?');
    params.push(entityId);
  }
  if (status) {
    clauses.push('status = ?');
    params.push(status);
  }

  const where = clauses.length ? `WHERE ${clauses.join(' AND ')}` : '';
  const rows = db
    .prepare(
      `SELECT id, entity_type, entity_id, remind_at, message, notify_member_ids,
              channel, status, created_at, updated_at
       FROM reminders ${where}
       ORDER BY remind_at ASC`
    )
    .all(...params);
  return rows.map(mapRow);
}

function getReminder(id) {
  const db = getDb();
  const row = db
    .prepare(
      `SELECT id, entity_type, entity_id, remind_at, message, notify_member_ids,
              channel, status, created_at, updated_at
       FROM reminders WHERE id = ?`
    )
    .get(id);
  return mapRow(row);
}

function createReminder(body) {
  const entityType = body.entityType || body.entity_type;
  const entityId = body.entityId || body.entity_id;
  const remindAt = body.remindAt || body.remind_at;

  if (!entityType || !ENTITY_TYPES.has(entityType)) {
    throw new Error('invalid-entity-type');
  }
  if (!entityId || !String(entityId).trim()) {
    throw new Error('entity-id-required');
  }
  if (!remindAt || !String(remindAt).trim()) {
    throw new Error('remind-at-required');
  }

  const notifyMemberIds = Array.isArray(body.notifyMemberIds)
    ? body.notifyMemberIds
    : Array.isArray(body.notify_member_ids)
      ? body.notify_member_ids
      : [];

  const id = newId();
  const db = getDb();
  db.prepare(
    `INSERT INTO reminders
       (id, entity_type, entity_id, remind_at, message, notify_member_ids, channel, status)
     VALUES (?, ?, ?, ?, ?, ?, ?, 'pending')`
  ).run(
    id,
    entityType,
    String(entityId).trim(),
    String(remindAt).trim(),
    clampText(body.message || '', 500),
    JSON.stringify(notifyMemberIds),
    body.channel || 'google_voice'
  );
  return getReminder(id);
}

function cancelReminder(id) {
  const existing = getReminder(id);
  if (!existing) return null;
  const db = getDb();
  db.prepare(
    `UPDATE reminders SET status = 'cancelled', updated_at = datetime('now') WHERE id = ?`
  ).run(id);
  return getReminder(id);
}

function deleteReminder(id) {
  const db = getDb();
  const result = db.prepare('DELETE FROM reminders WHERE id = ?').run(id);
  return result.changes > 0;
}

module.exports = {
  listReminders,
  getReminder,
  createReminder,
  cancelReminder,
  deleteReminder,
};
