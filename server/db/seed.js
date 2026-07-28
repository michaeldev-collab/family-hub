'use strict';

const crypto = require('crypto');
const { getDb } = require('./connection');
const { initDb } = require('./init');

function newId() {
  return crypto.randomUUID();
}

function seed() {
  initDb();
  const db = getDb();
  const count = db.prepare('SELECT COUNT(*) AS n FROM family_members').get().n;
  if (count > 0) {
    console.log('[seed] Members already exist — skipping seed.');
    return;
  }

  const members = [
    { id: newId(), name: 'Parent 1', color: '#4A90D9', emoji: '👨', order: 0 },
    { id: newId(), name: 'Parent 2', color: '#E67E22', emoji: '👩', order: 1 },
    { id: newId(), name: 'Kid 1', color: '#27AE60', emoji: '🧒', order: 2 },
    { id: newId(), name: 'Kid 2', color: '#9B59B6', emoji: '👧', order: 3 },
  ];

  const insertMember = db.prepare(
    `INSERT INTO family_members (id, name, color, avatar_emoji, sort_order)
     VALUES (@id, @name, @color, @emoji, @order)`
  );

  for (const m of members) {
    insertMember.run(m);
  }

  const today = new Date().toISOString().slice(0, 10);
  db.prepare(
    `INSERT INTO dinner_plans (date, cook_id, meal, main, side, side2, notes)
     VALUES (?, ?, ?, ?, ?, ?, ?)`
  ).run(
    today,
    members[0].id,
    'Tacos · Rice · Beans',
    'Tacos',
    'Rice',
    'Beans',
    'Kids help with prep'
  );

  db.prepare(
    `INSERT INTO grocery_items (id, text, category, added_by, list_key, needed)
     VALUES (?, ?, ?, ?, ?, ?)`
  ).run(newId(), 'Milk', 'dairy', members[0].id, 'main', 0);

  db.prepare(
    `INSERT INTO grocery_items (id, text, category, added_by, list_key, needed)
     VALUES (?, ?, ?, ?, ?, ?)`
  ).run(newId(), 'Bread', 'bakery', members[1].id, 'main', 0);

  db.prepare(
    `INSERT INTO grocery_items (id, text, category, added_by, list_key, needed)
     VALUES (?, ?, ?, ?, ?, ?)`
  ).run(newId(), 'Ketchup', 'staples', members[0].id, 'constant', 0);

  db.prepare(
    `INSERT INTO grocery_items (id, text, category, added_by, list_key, needed)
     VALUES (?, ?, ?, ?, ?, ?)`
  ).run(newId(), 'Butter', 'staples', members[0].id, 'constant', 0);

  db.prepare(
    `INSERT INTO chores (id, title, assignee_id, due_date)
     VALUES (?, ?, ?, ?)`
  ).run(newId(), 'Take out trash', members[2].id, today);

  db.prepare(
    `INSERT INTO chores (id, title, assignee_id, due_date)
     VALUES (?, ?, ?, ?)`
  ).run(newId(), 'Load dishwasher', members[3].id, today);

  db.prepare(`INSERT INTO notes (id, text, pinned) VALUES (?, ?, 1)`).run(
    newId(),
    'Soccer practice Thursday 5pm'
  );

  db.prepare(`INSERT INTO settings (key, value) VALUES ('timezone', 'America/Los_Angeles')`).run();

  console.log('[seed] Default family data inserted.');
}

if (require.main === module) {
  seed();
}

module.exports = { seed };
