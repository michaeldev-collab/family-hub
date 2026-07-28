'use strict';

const { getDb } = require('../db/connection');
const { clampText } = require('../utils/helpers');
const { bumpStateVersion } = require('./stateVersion');

function composeMeal(main, side, side2) {
  return [main, side, side2]
    .map((part) => String(part || '').trim())
    .filter(Boolean)
    .join(' · ');
}

function mapRow(row) {
  if (!row) return null;
  const main = row.main != null ? row.main : '';
  const side = row.side != null ? row.side : '';
  const side2 = row.side2 != null ? row.side2 : '';
  const legacyMeal = row.meal || '';
  const resolvedMain =
    main || side || side2 ? main : legacyMeal;
  const meal = composeMeal(resolvedMain, side, side2) || legacyMeal;
  return {
    date: row.date,
    cookId: row.cook_id,
    main: resolvedMain,
    side,
    side2,
    meal,
    notes: row.notes,
    updatedAt: row.updated_at,
  };
}

function getDinner(date) {
  const db = getDb();
  return mapRow(db.prepare('SELECT * FROM dinner_plans WHERE date = ?').get(date));
}

function setDinner(date, { cookId, meal, main, side, side2, notes } = {}) {
  const db = getDb();
  let safeMain = clampText(main != null ? main : '', 120);
  let safeSide = clampText(side != null ? side : '', 120);
  let safeSide2 = clampText(side2 != null ? side2 : '', 120);
  // Legacy clients still send `meal` alone — treat it as the main.
  if (!safeMain && !safeSide && !safeSide2 && meal != null) {
    safeMain = clampText(meal || '', 200);
  }
  const safeMeal = composeMeal(safeMain, safeSide, safeSide2);
  const safeNotes = clampText(notes || '', 500);
  db.prepare(
    `INSERT INTO dinner_plans (date, cook_id, meal, main, side, side2, notes, updated_at)
     VALUES (?, ?, ?, ?, ?, ?, ?, datetime('now'))
     ON CONFLICT(date) DO UPDATE SET
       cook_id = excluded.cook_id,
       meal = excluded.meal,
       main = excluded.main,
       side = excluded.side,
       side2 = excluded.side2,
       notes = excluded.notes,
       updated_at = datetime('now')`
  ).run(
    date,
    cookId || null,
    safeMeal,
    safeMain,
    safeSide,
    safeSide2,
    safeNotes
  );
  bumpStateVersion(db);
  return getDinner(date);
}

function listDinnerRange(startDate, endDate) {
  const db = getDb();
  return db
    .prepare(
      `SELECT * FROM dinner_plans
       WHERE date >= ? AND date <= ?
       ORDER BY date ASC`
    )
    .all(startDate, endDate)
    .map(mapRow);
}

module.exports = {
  composeMeal,
  getDinner,
  setDinner,
  listDinnerRange,
};
