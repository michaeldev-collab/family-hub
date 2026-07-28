'use strict';

const { getDb } = require('../db/connection');
const { newId, clampText, rowToBool } = require('../utils/helpers');
const { bumpStateVersion } = require('./stateVersion');

function mapRow(row) {
  if (!row) return null;
  return {
    id: row.id,
    text: row.text,
    pinned: rowToBool(row, 'pinned'),
    createdAt: row.created_at,
    updatedAt: row.updated_at,
  };
}

function listNotes() {
  const db = getDb();
  return db
    .prepare('SELECT * FROM notes ORDER BY pinned DESC, updated_at DESC')
    .all()
    .map(mapRow);
}

function getNote(id) {
  const db = getDb();
  return mapRow(db.prepare('SELECT * FROM notes WHERE id = ?').get(id));
}

function createNote({ text, pinned }) {
  const id = newId();
  const safeText = clampText(text, 1000);
  if (!safeText) throw new Error('text-required');
  const db = getDb();
  db.prepare(`INSERT INTO notes (id, text, pinned) VALUES (?, ?, ?)`).run(
    id,
    safeText,
    pinned ? 1 : 0
  );
  bumpStateVersion(db);
  return getNote(id);
}

function updateNote(id, patch) {
  const existing = getNote(id);
  if (!existing) return null;
  const db = getDb();
  db.prepare(
    `UPDATE notes
     SET text = ?, pinned = ?, updated_at = datetime('now')
     WHERE id = ?`
  ).run(
    patch.text !== undefined ? clampText(patch.text, 1000) : existing.text,
    patch.pinned !== undefined ? (patch.pinned ? 1 : 0) : (existing.pinned ? 1 : 0),
    id
  );
  bumpStateVersion(db);
  return getNote(id);
}

function deleteNote(id) {
  const db = getDb();
  const result = db.prepare('DELETE FROM notes WHERE id = ?').run(id);
  if (result.changes > 0) bumpStateVersion(db);
  return result.changes > 0;
}

module.exports = {
  listNotes,
  getNote,
  createNote,
  updateNote,
  deleteNote,
};
