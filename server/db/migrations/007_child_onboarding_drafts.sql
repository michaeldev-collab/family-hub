ALTER TABLE children ADD COLUMN birth_date TEXT
  CHECK (birth_date IS NULL OR birth_date GLOB '[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]');

ALTER TABLE display_settings ADD COLUMN behavior_goals_enabled INTEGER NOT NULL DEFAULT 1
  CHECK (behavior_goals_enabled IN (0, 1));
ALTER TABLE display_settings ADD COLUMN max_behavior_goals INTEGER NOT NULL DEFAULT 3
  CHECK (max_behavior_goals BETWEEN 0 AND 3);
ALTER TABLE display_child_profiles ADD COLUMN show_behavior_goals INTEGER
  CHECK (show_behavior_goals IS NULL OR show_behavior_goals IN (0, 1));

CREATE TABLE child_onboarding_drafts (
  id TEXT PRIMARY KEY,
  household_id TEXT NOT NULL REFERENCES households(id) ON DELETE RESTRICT,
  client_key TEXT NOT NULL,
  client_hash TEXT NOT NULL,
  schema_version INTEGER NOT NULL DEFAULT 1 CHECK (schema_version = 1),
  setup_mode TEXT NOT NULL DEFAULT 'quick' CHECK (setup_mode IN ('quick', 'advanced')),
  current_step TEXT NOT NULL DEFAULT 'profile' CHECK (current_step IN (
    'profile', 'schedule', 'behaviors', 'routines', 'daily_reward',
    'larger_reward', 'panel', 'review'
  )),
  completed_steps_json TEXT NOT NULL DEFAULT '[]',
  draft_json TEXT NOT NULL DEFAULT '{}',
  version INTEGER NOT NULL DEFAULT 1 CHECK (version > 0),
  status TEXT NOT NULL DEFAULT 'draft' CHECK (status IN ('draft', 'activating', 'activated', 'archived')),
  activation_key TEXT,
  activated_child_id TEXT UNIQUE,
  created_by TEXT NOT NULL,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now')),
  activated_at TEXT,
  UNIQUE (household_id, client_key),
  UNIQUE (household_id, activation_key),
  FOREIGN KEY (household_id, activated_child_id)
    REFERENCES child_households(household_id, child_id) ON DELETE RESTRICT
);

CREATE INDEX idx_onboarding_drafts_household
  ON child_onboarding_drafts(household_id, status, updated_at DESC);
CREATE INDEX idx_onboarding_drafts_activated_child
  ON child_onboarding_drafts(activated_child_id) WHERE activated_child_id IS NOT NULL;

CREATE TRIGGER child_onboarding_drafts_no_delete
BEFORE DELETE ON child_onboarding_drafts
BEGIN
  SELECT RAISE(ABORT, 'onboarding-draft-history-cannot-be-deleted');
END;
