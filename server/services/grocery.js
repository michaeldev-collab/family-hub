'use strict';

const { getDb } = require('../db/connection');
const { newId, clampText, rowToBool } = require('../utils/helpers');
const { bumpStateVersion } = require('./stateVersion');

const LIST_KEYS = new Set(['constant', 'main', 'other']);
const OTHER_TITLE_KEY = 'grocery_other_title';
const DEFAULT_OTHER_TITLE = 'Other';

function normalizeListKey(raw) {
  const key = String(raw || 'main').trim().toLowerCase();
  return LIST_KEYS.has(key) ? key : 'main';
}

function mapRow(row) {
  if (!row) return null;
  return {
    id: row.id,
    text: row.text,
    checked: rowToBool(row, 'checked'),
    needed: rowToBool(row, 'needed'),
    listKey: row.list_key || 'main',
    category: row.category,
    addedBy: row.added_by,
    createdAt: row.created_at,
    updatedAt: row.updated_at,
  };
}

function listGrocery({ includeChecked = true, listKey } = {}) {
  const db = getDb();
  const key = listKey ? normalizeListKey(listKey) : null;
  let sql = 'SELECT * FROM grocery_items';
  const params = [];
  const where = [];
  if (key) {
    where.push('list_key = ?');
    params.push(key);
  }
  if (!includeChecked) {
    // "Open" shopping items: needed constants, or unchecked main/other.
    where.push(
      `((list_key = 'constant' AND needed = 1) OR (list_key != 'constant' AND checked = 0))`
    );
  }
  if (where.length) sql += ` WHERE ${where.join(' AND ')}`;
  sql +=
    key === 'constant'
      ? ' ORDER BY needed DESC, text ASC'
      : ' ORDER BY checked ASC, created_at DESC';
  return db.prepare(sql).all(...params).map(mapRow);
}

function getGrocery(id) {
  const db = getDb();
  return mapRow(db.prepare('SELECT * FROM grocery_items WHERE id = ?').get(id));
}

function createGrocery({ text, category, addedBy, listKey, needed }) {
  const id = newId();
  const db = getDb();
  const safeText = clampText(text, 200);
  if (!safeText) throw new Error('text-required');
  const key = normalizeListKey(listKey);
  const neededVal = key === 'constant' && needed ? 1 : 0;
  db.prepare(
    `INSERT INTO grocery_items (id, text, category, added_by, list_key, needed)
     VALUES (?, ?, ?, ?, ?, ?)`
  ).run(
    id,
    safeText,
    clampText(category || 'general', 50),
    addedBy || null,
    key,
    neededVal
  );
  bumpStateVersion(db);
  return getGrocery(id);
}

function updateGrocery(id, patch) {
  const existing = getGrocery(id);
  if (!existing) return null;
  const db = getDb();
  const nextListKey =
    patch.listKey !== undefined ? normalizeListKey(patch.listKey) : existing.listKey;
  const checked =
    patch.checked !== undefined ? (patch.checked ? 1 : 0) : existing.checked ? 1 : 0;
  let needed =
    patch.needed !== undefined ? (patch.needed ? 1 : 0) : existing.needed ? 1 : 0;
  if (nextListKey !== 'constant') needed = 0;
  db.prepare(
    `UPDATE grocery_items
     SET text = ?, checked = ?, needed = ?, category = ?, list_key = ?,
         updated_at = datetime('now')
     WHERE id = ?`
  ).run(
    patch.text !== undefined ? clampText(patch.text, 200) : existing.text,
    nextListKey === 'constant' ? 0 : checked,
    needed,
    patch.category !== undefined ? clampText(patch.category, 50) : existing.category,
    nextListKey,
    id
  );
  bumpStateVersion(db);
  return getGrocery(id);
}

function toggleGroceryPanel(id) {
  const existing = getGrocery(id);
  if (!existing) return null;
  if (existing.listKey === 'constant') {
    return updateGrocery(id, { needed: !existing.needed });
  }
  return updateGrocery(id, { checked: !existing.checked });
}

function deleteGrocery(id) {
  const db = getDb();
  const result = db.prepare('DELETE FROM grocery_items WHERE id = ?').run(id);
  if (result.changes > 0) bumpStateVersion(db);
  return result.changes > 0;
}

function getOtherTitle() {
  const db = getDb();
  const row = db.prepare('SELECT value FROM settings WHERE key = ?').get(OTHER_TITLE_KEY);
  const title = row && row.value != null ? String(row.value).trim() : '';
  return title || DEFAULT_OTHER_TITLE;
}

function setOtherTitle(title) {
  const db = getDb();
  const safe = clampText(title || DEFAULT_OTHER_TITLE, 40) || DEFAULT_OTHER_TITLE;
  db.prepare(
    `INSERT INTO settings (key, value) VALUES (?, ?)
     ON CONFLICT(key) DO UPDATE SET value = excluded.value`
  ).run(OTHER_TITLE_KEY, safe);
  bumpStateVersion(db);
  return safe;
}

function shoppingItems({ limit = 24 } = {}) {
  return listGrocery({ includeChecked: false }).slice(0, limit);
}

module.exports = {
  LIST_KEYS,
  DEFAULT_OTHER_TITLE,
  listGrocery,
  getGrocery,
  createGrocery,
  updateGrocery,
  toggleGroceryPanel,
  deleteGrocery,
  getOtherTitle,
  setOtherTitle,
  shoppingItems,
};
