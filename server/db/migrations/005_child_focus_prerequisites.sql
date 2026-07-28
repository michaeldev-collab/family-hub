ALTER TABLE display_settings ADD COLUMN child_focus_enabled INTEGER NOT NULL DEFAULT 0
  CHECK (child_focus_enabled IN (0,1));
ALTER TABLE display_settings ADD COLUMN show_child_mode_toggle INTEGER NOT NULL DEFAULT 1
  CHECK (show_child_mode_toggle IN (0,1));
ALTER TABLE display_settings ADD COLUMN default_child_id TEXT
  REFERENCES children(id) ON DELETE SET NULL;
ALTER TABLE display_settings ADD COLUMN skip_child_selection_single INTEGER NOT NULL DEFAULT 0
  CHECK (skip_child_selection_single IN (0,1));
ALTER TABLE display_settings ADD COLUMN require_pin_exit INTEGER NOT NULL DEFAULT 1
  CHECK (require_pin_exit IN (0,1));
ALTER TABLE display_settings ADD COLUMN require_pin_change_child INTEGER NOT NULL DEFAULT 1
  CHECK (require_pin_change_child IN (0,1));
ALTER TABLE display_settings ADD COLUMN restart_mode TEXT NOT NULL DEFAULT 'restore'
  CHECK (restart_mode IN ('normal','restore','child'));
ALTER TABLE display_settings ADD COLUMN auto_return_enabled INTEGER NOT NULL DEFAULT 1
  CHECK (auto_return_enabled IN (0,1));
ALTER TABLE display_settings ADD COLUMN child_dashboard_timeout_seconds INTEGER NOT NULL DEFAULT 300
  CHECK (child_dashboard_timeout_seconds BETWEEN 30 AND 86400);
ALTER TABLE display_settings ADD COLUMN show_task_grid INTEGER NOT NULL DEFAULT 1
  CHECK (show_task_grid IN (0,1));
ALTER TABLE display_settings ADD COLUMN sync_interval_seconds INTEGER NOT NULL DEFAULT 60
  CHECK (sync_interval_seconds BETWEEN 15 AND 3600);

CREATE TABLE display_child_profiles (
  device_id TEXT NOT NULL REFERENCES display_settings(device_id) ON DELETE RESTRICT,
  child_id TEXT NOT NULL REFERENCES children(id) ON DELETE RESTRICT,
  enabled INTEGER NOT NULL DEFAULT 1 CHECK (enabled IN (0,1)),
  display_order INTEGER NOT NULL DEFAULT 0,
  page_preset TEXT NOT NULL DEFAULT 'standard'
    CHECK (page_preset IN ('toddler','standard','independent')),
  show_task_grid INTEGER CHECK (show_task_grid IS NULL OR show_task_grid IN (0,1)),
  visual_only_mode INTEGER CHECK (visual_only_mode IS NULL OR visual_only_mode IN (0,1)),
  show_exact_star_count INTEGER NOT NULL DEFAULT 1
    CHECK (show_exact_star_count IN (0,1)),
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now')),
  PRIMARY KEY (device_id, child_id)
);

CREATE TABLE child_focus_sessions (
  id TEXT PRIMARY KEY,
  device_id TEXT NOT NULL REFERENCES display_settings(device_id) ON DELETE RESTRICT,
  child_id TEXT NOT NULL REFERENCES children(id) ON DELETE RESTRICT,
  task_assignment_id TEXT REFERENCES task_assignments(id) ON DELETE RESTRICT,
  then_reward_id TEXT REFERENCES reward_definitions(id) ON DELETE RESTRICT,
  first_media_asset_id TEXT REFERENCES media_assets(id) ON DELETE SET NULL,
  then_media_asset_id TEXT REFERENCES media_assets(id) ON DELETE SET NULL,
  first_label TEXT NOT NULL DEFAULT '',
  then_label TEXT NOT NULL DEFAULT '',
  end_condition TEXT NOT NULL DEFAULT 'manual'
    CHECK (end_condition IN ('manual','task_approved','time')),
  ends_at TEXT,
  status TEXT NOT NULL DEFAULT 'active'
    CHECK (status IN ('active','completed','cancelled','expired')),
  created_by_session TEXT,
  ended_by_session TEXT,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  ended_at TEXT,
  updated_at TEXT NOT NULL DEFAULT (datetime('now')),
  CHECK (end_condition != 'time' OR ends_at IS NOT NULL)
);

CREATE UNIQUE INDEX uq_child_focus_one_active
  ON child_focus_sessions(device_id, child_id) WHERE status='active';
CREATE INDEX idx_display_child_profiles_order
  ON display_child_profiles(device_id, enabled, display_order, child_id);
CREATE INDEX idx_child_focus_active
  ON child_focus_sessions(device_id, child_id, status, created_at DESC);

CREATE TRIGGER child_focus_sessions_no_delete
BEFORE DELETE ON child_focus_sessions
BEGIN
  SELECT RAISE(ABORT, 'child-focus-history-cannot-be-deleted');
END;
