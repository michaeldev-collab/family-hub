-- Family Hub v0.1 schema

CREATE TABLE IF NOT EXISTS family_members (
  id TEXT PRIMARY KEY,
  name TEXT NOT NULL,
  color TEXT NOT NULL DEFAULT '#4A90D9',
  avatar_emoji TEXT NOT NULL DEFAULT '👤',
  sort_order INTEGER NOT NULL DEFAULT 0,
  phone TEXT,
  notify_enabled INTEGER NOT NULL DEFAULT 1,
  clerk_user_id TEXT UNIQUE,
  created_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS grocery_items (
  id TEXT PRIMARY KEY,
  text TEXT NOT NULL,
  checked INTEGER NOT NULL DEFAULT 0,
  needed INTEGER NOT NULL DEFAULT 0,
  list_key TEXT NOT NULL DEFAULT 'main',
  category TEXT NOT NULL DEFAULT 'general',
  added_by TEXT,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now')),
  FOREIGN KEY (added_by) REFERENCES family_members(id) ON DELETE SET NULL
);

CREATE TABLE IF NOT EXISTS chores (
  id TEXT PRIMARY KEY,
  title TEXT NOT NULL,
  assignee_id TEXT,
  due_date TEXT,
  recurrence TEXT,
  completed INTEGER NOT NULL DEFAULT 0,
  completed_at TEXT,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now')),
  FOREIGN KEY (assignee_id) REFERENCES family_members(id) ON DELETE SET NULL
);

CREATE TABLE IF NOT EXISTS dinner_plans (
  date TEXT PRIMARY KEY,
  cook_id TEXT,
  meal TEXT NOT NULL DEFAULT '',
  main TEXT NOT NULL DEFAULT '',
  side TEXT NOT NULL DEFAULT '',
  side2 TEXT NOT NULL DEFAULT '',
  notes TEXT NOT NULL DEFAULT '',
  updated_at TEXT NOT NULL DEFAULT (datetime('now')),
  FOREIGN KEY (cook_id) REFERENCES family_members(id) ON DELETE SET NULL
);

CREATE TABLE IF NOT EXISTS notes (
  id TEXT PRIMARY KEY,
  text TEXT NOT NULL,
  pinned INTEGER NOT NULL DEFAULT 0,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS events (
  -- v0.1: calendar schema reserved; no calendar API or UI yet (deferred to v0.2)
  id TEXT PRIMARY KEY,
  title TEXT NOT NULL,
  start_at TEXT NOT NULL,
  end_at TEXT,
  all_day INTEGER NOT NULL DEFAULT 0,
  member_ids TEXT NOT NULL DEFAULT '[]',
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);
-- v0.1: calendar/events API deferred to v0.2 — table reserved for future use

CREATE TABLE IF NOT EXISTS settings (
  key TEXT PRIMARY KEY,
  value TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS idempotency_keys (
  event_id TEXT PRIMARY KEY,
  response_json TEXT NOT NULL,
  created_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS write_log (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  action TEXT NOT NULL,
  entity TEXT,
  entity_id TEXT,
  source TEXT NOT NULL DEFAULT 'api',
  created_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE INDEX IF NOT EXISTS idx_grocery_checked ON grocery_items(checked);
CREATE INDEX IF NOT EXISTS idx_chores_completed ON chores(completed);
CREATE INDEX IF NOT EXISTS idx_chores_due ON chores(due_date);
CREATE INDEX IF NOT EXISTS idx_notes_pinned ON notes(pinned);

CREATE TABLE IF NOT EXISTS reminders (
  id TEXT PRIMARY KEY,
  entity_type TEXT NOT NULL CHECK (entity_type IN ('grocery', 'chore', 'dinner', 'note')),
  entity_id TEXT NOT NULL,
  remind_at TEXT NOT NULL,
  message TEXT NOT NULL DEFAULT '',
  notify_member_ids TEXT NOT NULL DEFAULT '[]',
  channel TEXT NOT NULL DEFAULT 'google_voice',
  status TEXT NOT NULL DEFAULT 'pending' CHECK (status IN ('pending', 'sent', 'cancelled')),
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE INDEX IF NOT EXISTS idx_reminders_entity ON reminders(entity_type, entity_id);
CREATE INDEX IF NOT EXISTS idx_reminders_status_at ON reminders(status, remind_at);
