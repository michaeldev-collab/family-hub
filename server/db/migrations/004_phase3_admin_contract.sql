CREATE TABLE routine_groups (
  id TEXT PRIMARY KEY,
  name TEXT NOT NULL,
  child_facing_label TEXT NOT NULL DEFAULT '',
  media_asset_id TEXT REFERENCES media_assets(id) ON DELETE SET NULL,
  active INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0,1)),
  archived_at TEXT,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE routine_group_tasks (
  routine_group_id TEXT NOT NULL REFERENCES routine_groups(id) ON DELETE RESTRICT,
  task_id TEXT NOT NULL REFERENCES task_definitions(id) ON DELETE RESTRICT,
  display_order INTEGER NOT NULL DEFAULT 0,
  required INTEGER NOT NULL DEFAULT 0 CHECK (required IN (0,1)),
  active INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0,1)),
  PRIMARY KEY (routine_group_id, task_id)
);

ALTER TABLE task_assignments ADD COLUMN exceptions_config TEXT NOT NULL DEFAULT '{}';
ALTER TABLE task_completion_requests ADD COLUMN resolution_type TEXT
  CHECK (resolution_type IS NULL OR resolution_type IN ('approved','rejected','excused'));
ALTER TABLE task_completion_requests ADD COLUMN parent_note TEXT;

ALTER TABLE reward_definitions ADD COLUMN child_audio_ref TEXT;
ALTER TABLE reward_definitions ADD COLUMN usage_limit INTEGER
  CHECK (usage_limit IS NULL OR usage_limit > 0);
ALTER TABLE reward_definitions ADD COLUMN availability_config TEXT NOT NULL DEFAULT '{}';
ALTER TABLE reward_definitions ADD COLUMN display_priority INTEGER NOT NULL DEFAULT 0;

ALTER TABLE reward_terms ADD COLUMN enabled INTEGER NOT NULL DEFAULT 1 CHECK (enabled IN (0,1));
ALTER TABLE reward_terms ADD COLUMN reset_behavior TEXT NOT NULL DEFAULT 'manual'
  CHECK (reset_behavior IN ('manual','archive_and_restart','reset_progress'));
ALTER TABLE reward_terms ADD COLUMN automatic_restart INTEGER NOT NULL DEFAULT 0
  CHECK (automatic_restart IN (0,1));
ALTER TABLE reward_terms ADD COLUMN approval_required INTEGER NOT NULL DEFAULT 1
  CHECK (approval_required IN (0,1));

ALTER TABLE consequences ADD COLUMN cancelled_by TEXT REFERENCES family_members(id) ON DELETE SET NULL;
ALTER TABLE consequences ADD COLUMN cancelled_at TEXT;
ALTER TABLE consequences ADD COLUMN cancellation_reason TEXT;

ALTER TABLE display_settings ADD COLUMN show_child_profiles INTEGER NOT NULL DEFAULT 1 CHECK (show_child_profiles IN (0,1));
ALTER TABLE display_settings ADD COLUMN visual_only_mode INTEGER NOT NULL DEFAULT 0 CHECK (visual_only_mode IN (0,1));
ALTER TABLE display_settings ADD COLUMN show_consequence_indicator INTEGER NOT NULL DEFAULT 1 CHECK (show_consequence_indicator IN (0,1));
ALTER TABLE display_settings ADD COLUMN profile_lock_timeout_seconds INTEGER NOT NULL DEFAULT 0 CHECK (profile_lock_timeout_seconds >= 0);
ALTER TABLE display_settings ADD COLUMN cache_refresh_seconds INTEGER NOT NULL DEFAULT 300 CHECK (cache_refresh_seconds >= 30);
ALTER TABLE display_settings ADD COLUMN reward_ordering TEXT NOT NULL DEFAULT 'priority'
  CHECK (reward_ordering IN ('priority','name','cost'));

ALTER TABLE media_assets ADD COLUMN logical_asset_id TEXT;
ALTER TABLE media_assets ADD COLUMN replaces_id TEXT REFERENCES media_assets(id) ON DELETE RESTRICT;
CREATE UNIQUE INDEX uq_media_logical_version
  ON media_assets(logical_asset_id, version) WHERE logical_asset_id IS NOT NULL;
CREATE INDEX idx_routine_groups_active ON routine_groups(active, name);
CREATE INDEX idx_routine_group_tasks_order ON routine_group_tasks(routine_group_id, display_order);
CREATE INDEX idx_rewards_priority ON reward_definitions(active, display_priority, name);
CREATE INDEX idx_media_logical ON media_assets(logical_asset_id, version DESC);

CREATE TRIGGER routine_groups_no_delete
BEFORE DELETE ON routine_groups
BEGIN
  SELECT RAISE(ABORT, 'routine-history-cannot-be-deleted');
END;
