'use strict';

const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const { getDb } = require('./connection');

const MIGRATIONS_DIR = path.join(__dirname, 'migrations');

function columnExists(db, table, column) {
  const cols = db.prepare(`PRAGMA table_info(${table})`).all();
  return cols.some((c) => c.name === column);
}

function migrateDb(db) {
  if (!columnExists(db, 'family_members', 'phone')) {
    db.exec('ALTER TABLE family_members ADD COLUMN phone TEXT');
  }
  if (!columnExists(db, 'family_members', 'notify_enabled')) {
    db.exec(
      'ALTER TABLE family_members ADD COLUMN notify_enabled INTEGER NOT NULL DEFAULT 1'
    );
  }
  if (!columnExists(db, 'family_members', 'clerk_user_id')) {
    db.exec('ALTER TABLE family_members ADD COLUMN clerk_user_id TEXT');
  }
  // Unique index for Clerk link (SQLite allows multiple NULLs).
  db.exec(
    `CREATE UNIQUE INDEX IF NOT EXISTS idx_family_members_clerk_user_id
     ON family_members(clerk_user_id)
     WHERE clerk_user_id IS NOT NULL`
  );
  if (!columnExists(db, 'dinner_plans', 'main')) {
    db.exec("ALTER TABLE dinner_plans ADD COLUMN main TEXT NOT NULL DEFAULT ''");
  }
  if (!columnExists(db, 'dinner_plans', 'side')) {
    db.exec("ALTER TABLE dinner_plans ADD COLUMN side TEXT NOT NULL DEFAULT ''");
  }
  if (!columnExists(db, 'dinner_plans', 'side2')) {
    db.exec("ALTER TABLE dinner_plans ADD COLUMN side2 TEXT NOT NULL DEFAULT ''");
  }
  // Legacy single-line meals become the main dish when structured fields are empty.
  db.exec(
    `UPDATE dinner_plans
     SET main = meal
     WHERE IFNULL(TRIM(main), '') = ''
       AND IFNULL(TRIM(side), '') = ''
       AND IFNULL(TRIM(side2), '') = ''
       AND IFNULL(TRIM(meal), '') != ''`
  );
  if (!columnExists(db, 'grocery_items', 'list_key')) {
    db.exec("ALTER TABLE grocery_items ADD COLUMN list_key TEXT NOT NULL DEFAULT 'main'");
  }
  if (!columnExists(db, 'grocery_items', 'needed')) {
    db.exec('ALTER TABLE grocery_items ADD COLUMN needed INTEGER NOT NULL DEFAULT 0');
  }
  db.prepare(
    `INSERT INTO settings (key, value) VALUES ('grocery_other_title', 'Other')
     ON CONFLICT(key) DO NOTHING`
  ).run();
}

function migrationFiles() {
  if (!fs.existsSync(MIGRATIONS_DIR)) return [];
  return fs
    .readdirSync(MIGRATIONS_DIR)
    .filter((name) => /^\d{3}_[a-z0-9_]+\.sql$/.test(name))
    .sort();
}

function migrationVersion(name) {
  return Number.parseInt(name.slice(0, 3), 10);
}

function migrationChecksum(sql) {
  return crypto.createHash('sha256').update(sql).digest('hex');
}

function ensureMigrationLedger(db) {
  db.exec(`
    CREATE TABLE IF NOT EXISTS schema_migrations (
      version INTEGER PRIMARY KEY,
      name TEXT NOT NULL UNIQUE,
      checksum TEXT NOT NULL,
      applied_at TEXT NOT NULL DEFAULT (datetime('now'))
    )
  `);
}

function applyMigrations(db, options = {}) {
  const migrationsDir = options.migrationsDir || MIGRATIONS_DIR;
  ensureMigrationLedger(db);
  const files = fs.existsSync(migrationsDir)
    ? fs.readdirSync(migrationsDir).filter((name) => /^\d{3}_[a-z0-9_]+\.sql$/.test(name)).sort()
    : [];
  const versions = files.map(migrationVersion);
  if (new Set(versions).size !== versions.length) {
    throw new Error('duplicate-migration-version');
  }

  const applied = new Map(
    db.prepare('SELECT version, name, checksum FROM schema_migrations ORDER BY version').all()
      .map((row) => [Number(row.version), row])
  );
  const maxKnown = versions.length ? Math.max(...versions) : 0;
  for (const row of applied.values()) {
    if (Number(row.version) > maxKnown || !versions.includes(Number(row.version))) {
      throw new Error(`unknown-migration-version:${row.version}`);
    }
  }

  let appliedCount = 0;
  for (const file of files) {
    const version = migrationVersion(file);
    const sql = fs.readFileSync(path.join(migrationsDir, file), 'utf8');
    const checksum = migrationChecksum(sql);
    const previous = applied.get(version);
    if (previous) {
      if (previous.name !== file || previous.checksum !== checksum) {
        throw new Error(`migration-drift:${file}`);
      }
      continue;
    }

    db.exec('BEGIN IMMEDIATE');
    try {
      db.exec(sql);
      db.prepare(
        'INSERT INTO schema_migrations (version, name, checksum) VALUES (?, ?, ?)'
      ).run(version, file, checksum);
      db.exec(`PRAGMA user_version = ${version}`);
      db.exec('COMMIT');
      appliedCount += 1;
    } catch (err) {
      db.exec('ROLLBACK');
      throw err;
    }
  }
  return appliedCount;
}

function initDb() {
  const db = getDb();
  const schema = fs.readFileSync(path.join(__dirname, 'schema.sql'), 'utf8');
  db.exec(schema);
  migrateDb(db);
  applyMigrations(db);
  const { ensureStateVersion } = require('../services/stateVersion');
  ensureStateVersion(db);
  return db;
}

if (require.main === module) {
  initDb();
  console.log('[db] Schema initialized.');
}

module.exports = {
  MIGRATIONS_DIR,
  initDb,
  migrateDb,
  migrationFiles,
  migrationChecksum,
  applyMigrations,
};
