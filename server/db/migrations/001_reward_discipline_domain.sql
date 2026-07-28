CREATE TABLE media_assets (
  id TEXT PRIMARY KEY,
  asset_type TEXT NOT NULL CHECK (asset_type IN ('builtin', 'child_profile', 'task', 'reward', 'rule', 'consequence')),
  original_path TEXT,
  panel_path TEXT,
  thumbnail_path TEXT,
  mime_type TEXT NOT NULL,
  width INTEGER NOT NULL CHECK (width > 0),
  height INTEGER NOT NULL CHECK (height > 0),
  version INTEGER NOT NULL DEFAULT 1 CHECK (version > 0),
  active INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0, 1)),
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE children (
  id TEXT PRIMARY KEY REFERENCES family_members(id) ON DELETE RESTRICT,
  display_name TEXT NOT NULL,
  profile_asset_id TEXT REFERENCES media_assets(id) ON DELETE SET NULL,
  age_mode TEXT NOT NULL DEFAULT 'toddler' CHECK (age_mode IN ('toddler', 'preschool', 'reader', 'custom')),
  display_color TEXT NOT NULL DEFAULT '#4A90D9',
  screen_visible INTEGER NOT NULL DEFAULT 1 CHECK (screen_visible IN (0, 1)),
  active INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0, 1)),
  archived_at TEXT,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE child_attendance_rules (
  id TEXT PRIMARY KEY,
  child_id TEXT NOT NULL UNIQUE REFERENCES children(id) ON DELETE RESTRICT,
  attendance_type TEXT NOT NULL CHECK (attendance_type IN ('always', 'scheduled', 'manual')),
  scheduled_days TEXT NOT NULL DEFAULT '[]',
  manual_attendance_enabled INTEGER NOT NULL DEFAULT 0 CHECK (manual_attendance_enabled IN (0, 1)),
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE task_definitions (
  id TEXT PRIMARY KEY,
  name TEXT NOT NULL,
  child_facing_label TEXT NOT NULL DEFAULT '',
  media_asset_id TEXT REFERENCES media_assets(id) ON DELETE SET NULL,
  category TEXT NOT NULL DEFAULT 'routine',
  default_star_value INTEGER NOT NULL DEFAULT 0 CHECK (default_star_value >= 0),
  default_daily_progress REAL NOT NULL DEFAULT 0 CHECK (default_daily_progress >= 0),
  requires_approval INTEGER NOT NULL DEFAULT 1 CHECK (requires_approval IN (0, 1)),
  active INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0, 1)),
  archived_at TEXT,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE task_assignments (
  id TEXT PRIMARY KEY,
  task_id TEXT NOT NULL REFERENCES task_definitions(id) ON DELETE RESTRICT,
  child_id TEXT NOT NULL REFERENCES children(id) ON DELETE RESTRICT,
  schedule_type TEXT NOT NULL CHECK (schedule_type IN ('daily', 'selected_days', 'date_range', 'manual')),
  schedule_config TEXT NOT NULL DEFAULT '{}',
  available_at TEXT,
  due_at TEXT,
  required INTEGER NOT NULL DEFAULT 0 CHECK (required IN (0, 1)),
  display_order INTEGER NOT NULL DEFAULT 0,
  active INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0, 1)),
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now')),
  UNIQUE (task_id, child_id, schedule_type, schedule_config)
);

CREATE TABLE task_completion_requests (
  id TEXT PRIMARY KEY,
  assignment_id TEXT NOT NULL REFERENCES task_assignments(id) ON DELETE RESTRICT,
  child_id TEXT NOT NULL REFERENCES children(id) ON DELETE RESTRICT,
  request_key TEXT NOT NULL UNIQUE,
  requested_at TEXT NOT NULL DEFAULT (datetime('now')),
  requested_from TEXT NOT NULL CHECK (requested_from IN ('panel', 'parent', 'api')),
  status TEXT NOT NULL DEFAULT 'awaiting_parent' CHECK (status IN ('requested', 'awaiting_parent', 'approved', 'rejected', 'cancelled', 'expired')),
  approved_by TEXT REFERENCES family_members(id) ON DELETE SET NULL,
  approved_at TEXT,
  rejection_reason TEXT,
  updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE reward_definitions (
  id TEXT PRIMARY KEY,
  name TEXT NOT NULL,
  media_asset_id TEXT REFERENCES media_assets(id) ON DELETE SET NULL,
  reward_type TEXT NOT NULL CHECK (reward_type IN ('daily', 'term', 'spendable', 'milestone')),
  star_cost INTEGER NOT NULL DEFAULT 0 CHECK (star_cost >= 0),
  repeatable INTEGER NOT NULL DEFAULT 1 CHECK (repeatable IN (0, 1)),
  approval_required INTEGER NOT NULL DEFAULT 1 CHECK (approval_required IN (0, 1)),
  active INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0, 1)),
  archived_at TEXT,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE reward_child_access (
  id TEXT PRIMARY KEY,
  reward_id TEXT NOT NULL REFERENCES reward_definitions(id) ON DELETE RESTRICT,
  child_id TEXT NOT NULL REFERENCES children(id) ON DELETE RESTRICT,
  available INTEGER NOT NULL DEFAULT 1 CHECK (available IN (0, 1)),
  display_order INTEGER NOT NULL DEFAULT 0,
  custom_cost INTEGER CHECK (custom_cost IS NULL OR custom_cost >= 0),
  UNIQUE (reward_id, child_id)
);

CREATE TABLE reward_terms (
  id TEXT PRIMARY KEY,
  child_id TEXT NOT NULL REFERENCES children(id) ON DELETE RESTRICT,
  name TEXT NOT NULL,
  term_type TEXT NOT NULL CHECK (term_type IN ('fixed_length', 'scheduled_day', 'attended_day', 'calendar_week', 'rolling', 'custom_date', 'manual')),
  term_config TEXT NOT NULL DEFAULT '{}',
  required_successful_days INTEGER NOT NULL DEFAULT 0 CHECK (required_successful_days >= 0),
  required_progress REAL NOT NULL DEFAULT 0 CHECK (required_progress >= 0),
  allow_partial INTEGER NOT NULL DEFAULT 0 CHECK (allow_partial IN (0, 1)),
  allow_recovery INTEGER NOT NULL DEFAULT 1 CHECK (allow_recovery IN (0, 1)),
  daily_reward_id TEXT REFERENCES reward_definitions(id) ON DELETE SET NULL,
  term_reward_id TEXT REFERENCES reward_definitions(id) ON DELETE SET NULL,
  status TEXT NOT NULL DEFAULT 'draft' CHECK (status IN ('draft', 'active', 'paused', 'completed', 'ended', 'reset', 'archived')),
  starts_at TEXT,
  ends_at TEXT,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE UNIQUE INDEX uq_reward_terms_one_current
  ON reward_terms(child_id) WHERE status IN ('active', 'paused');

CREATE TABLE reward_term_days (
  id TEXT PRIMARY KEY,
  term_id TEXT NOT NULL REFERENCES reward_terms(id) ON DELETE RESTRICT,
  calendar_date TEXT NOT NULL,
  attendance_status TEXT NOT NULL DEFAULT 'unknown' CHECK (attendance_status IN ('unknown', 'present', 'not_present', 'excused')),
  day_status TEXT NOT NULL DEFAULT 'pending' CHECK (day_status IN ('pending', 'successful', 'partially_successful', 'recovery_completed', 'unsuccessful', 'excused', 'not_present', 'term_paused')),
  progress_value REAL NOT NULL DEFAULT 0 CHECK (progress_value >= 0),
  daily_progress_value REAL NOT NULL DEFAULT 0 CHECK (daily_progress_value >= 0),
  approved_by TEXT REFERENCES family_members(id) ON DELETE SET NULL,
  approved_at TEXT,
  note TEXT,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now')),
  UNIQUE (term_id, calendar_date)
);

CREATE TABLE child_reward_goals (
  id TEXT PRIMARY KEY,
  child_id TEXT NOT NULL REFERENCES children(id) ON DELETE RESTRICT,
  reward_id TEXT NOT NULL REFERENCES reward_definitions(id) ON DELETE RESTRICT,
  goal_type TEXT NOT NULL CHECK (goal_type IN ('daily', 'term', 'spendable', 'milestone')),
  status TEXT NOT NULL DEFAULT 'selected' CHECK (status IN ('selected', 'in_progress', 'ready', 'requested', 'completed', 'cancelled', 'expired')),
  selected_at TEXT NOT NULL DEFAULT (datetime('now')),
  completed_at TEXT,
  updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE reward_redemptions (
  id TEXT PRIMARY KEY,
  child_id TEXT NOT NULL REFERENCES children(id) ON DELETE RESTRICT,
  reward_id TEXT NOT NULL REFERENCES reward_definitions(id) ON DELETE RESTRICT,
  term_id TEXT REFERENCES reward_terms(id) ON DELETE SET NULL,
  request_key TEXT NOT NULL UNIQUE,
  requested_at TEXT NOT NULL DEFAULT (datetime('now')),
  status TEXT NOT NULL DEFAULT 'requested' CHECK (status IN ('requested', 'approved', 'declined', 'scheduled', 'redeemed', 'completed', 'expired', 'cancelled')),
  approved_by TEXT REFERENCES family_members(id) ON DELETE SET NULL,
  approved_at TEXT,
  scheduled_for TEXT,
  completed_at TEXT,
  star_cost INTEGER NOT NULL DEFAULT 0 CHECK (star_cost >= 0),
  updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE star_transactions (
  id TEXT PRIMARY KEY,
  child_id TEXT NOT NULL REFERENCES children(id) ON DELETE RESTRICT,
  amount INTEGER NOT NULL CHECK (amount != 0),
  transaction_type TEXT NOT NULL CHECK (transaction_type IN ('award', 'bonus', 'reward_redemption', 'administrative_correction', 'reversal')),
  category TEXT NOT NULL DEFAULT 'general',
  reason TEXT NOT NULL,
  task_completion_id TEXT REFERENCES task_completion_requests(id) ON DELETE RESTRICT,
  reward_redemption_id TEXT REFERENCES reward_redemptions(id) ON DELETE RESTRICT,
  issued_by TEXT REFERENCES family_members(id) ON DELETE SET NULL,
  reversal_of TEXT REFERENCES star_transactions(id) ON DELETE RESTRICT,
  created_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE UNIQUE INDEX uq_star_transaction_task_award
  ON star_transactions(task_completion_id)
  WHERE task_completion_id IS NOT NULL AND transaction_type = 'award';
CREATE UNIQUE INDEX uq_star_transaction_redemption
  ON star_transactions(reward_redemption_id)
  WHERE reward_redemption_id IS NOT NULL AND transaction_type = 'reward_redemption';
CREATE UNIQUE INDEX uq_star_transaction_reversal
  ON star_transactions(reversal_of) WHERE reversal_of IS NOT NULL;

CREATE TABLE behavior_rules (
  id TEXT PRIMARY KEY,
  name TEXT NOT NULL,
  child_facing_label TEXT NOT NULL DEFAULT '',
  media_asset_id TEXT REFERENCES media_assets(id) ON DELETE SET NULL,
  parent_description TEXT NOT NULL DEFAULT '',
  default_severity TEXT NOT NULL DEFAULT 'low' CHECK (default_severity IN ('low', 'medium', 'high', 'safety')),
  active INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0, 1)),
  archived_at TEXT,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE behavior_incidents (
  id TEXT PRIMARY KEY,
  child_id TEXT NOT NULL REFERENCES children(id) ON DELETE RESTRICT,
  rule_id TEXT REFERENCES behavior_rules(id) ON DELETE SET NULL,
  severity TEXT NOT NULL CHECK (severity IN ('low', 'medium', 'high', 'safety')),
  parent_note TEXT,
  child_message TEXT NOT NULL DEFAULT '',
  recorded_by TEXT REFERENCES family_members(id) ON DELETE SET NULL,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  acknowledged_at TEXT,
  resolved_at TEXT,
  status TEXT NOT NULL DEFAULT 'open' CHECK (status IN ('open', 'acknowledged', 'recovery_pending', 'resolved', 'archived')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE consequences (
  id TEXT PRIMARY KEY,
  incident_id TEXT NOT NULL REFERENCES behavior_incidents(id) ON DELETE RESTRICT,
  consequence_type TEXT NOT NULL CHECK (consequence_type IN ('warning_only', 'privilege_pause', 'item_restriction', 'corrective_task', 'calm_down', 'activity_pause', 'parent_defined', 'time_based', 'action_based', 'time_and_action')),
  media_asset_id TEXT REFERENCES media_assets(id) ON DELETE SET NULL,
  child_message TEXT NOT NULL DEFAULT '',
  parent_description TEXT NOT NULL DEFAULT '',
  starts_at TEXT NOT NULL DEFAULT (datetime('now')),
  ends_at TEXT,
  completion_requirement TEXT,
  completed_at TEXT,
  resolved_by TEXT REFERENCES family_members(id) ON DELETE SET NULL,
  status TEXT NOT NULL DEFAULT 'active' CHECK (status IN ('active', 'recovery_pending', 'completed', 'resolved', 'cancelled', 'expired')),
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE display_settings (
  id TEXT PRIMARY KEY,
  device_id TEXT NOT NULL UNIQUE,
  reward_module_enabled INTEGER NOT NULL DEFAULT 0 CHECK (reward_module_enabled IN (0, 1)),
  show_star_totals INTEGER NOT NULL DEFAULT 1 CHECK (show_star_totals IN (0, 1)),
  show_daily_reward INTEGER NOT NULL DEFAULT 1 CHECK (show_daily_reward IN (0, 1)),
  show_term_reward INTEGER NOT NULL DEFAULT 1 CHECK (show_term_reward IN (0, 1)),
  allow_task_requests INTEGER NOT NULL DEFAULT 1 CHECK (allow_task_requests IN (0, 1)),
  allow_reward_selection INTEGER NOT NULL DEFAULT 0 CHECK (allow_reward_selection IN (0, 1)),
  allow_reward_requests INTEGER NOT NULL DEFAULT 0 CHECK (allow_reward_requests IN (0, 1)),
  sound_enabled INTEGER NOT NULL DEFAULT 0 CHECK (sound_enabled IN (0, 1)),
  animation_enabled INTEGER NOT NULL DEFAULT 0 CHECK (animation_enabled IN (0, 1)),
  screen_timeout_seconds INTEGER NOT NULL DEFAULT 0 CHECK (screen_timeout_seconds >= 0),
  visible_reward_limit INTEGER NOT NULL DEFAULT 6 CHECK (visible_reward_limit BETWEEN 1 AND 24),
  image_quality_preset TEXT NOT NULL DEFAULT 'balanced' CHECK (image_quality_preset IN ('low', 'balanced', 'high')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE domain_idempotency_keys (
  action_scope TEXT NOT NULL,
  idempotency_key TEXT NOT NULL,
  actor_type TEXT NOT NULL CHECK (actor_type IN ('parent', 'panel', 'system')),
  actor_id TEXT,
  response_json TEXT NOT NULL,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  PRIMARY KEY (action_scope, idempotency_key)
);

CREATE TABLE audit_events (
  id TEXT PRIMARY KEY,
  event_type TEXT NOT NULL,
  entity_type TEXT NOT NULL,
  entity_id TEXT,
  actor_type TEXT NOT NULL CHECK (actor_type IN ('parent', 'panel', 'system')),
  actor_id TEXT,
  source TEXT NOT NULL DEFAULT 'api',
  metadata_json TEXT NOT NULL DEFAULT '{}',
  created_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE INDEX idx_children_active ON children(active, display_name);
CREATE INDEX idx_task_assignments_child ON task_assignments(child_id, active, available_at, due_at);
CREATE INDEX idx_completion_requests_queue ON task_completion_requests(status, requested_at);
CREATE INDEX idx_completion_requests_child ON task_completion_requests(child_id, status);
CREATE INDEX idx_reward_access_child ON reward_child_access(child_id, available, display_order);
CREATE INDEX idx_reward_terms_child ON reward_terms(child_id, status, starts_at);
CREATE INDEX idx_reward_term_days_term ON reward_term_days(term_id, calendar_date);
CREATE INDEX idx_reward_goals_child ON child_reward_goals(child_id, status);
CREATE INDEX idx_reward_redemptions_queue ON reward_redemptions(status, requested_at);
CREATE INDEX idx_reward_redemptions_child ON reward_redemptions(child_id, status);
CREATE INDEX idx_star_transactions_child ON star_transactions(child_id, created_at);
CREATE INDEX idx_incidents_child ON behavior_incidents(child_id, status, created_at);
CREATE INDEX idx_consequences_incident ON consequences(incident_id);
CREATE INDEX idx_consequences_active ON consequences(status, ends_at);
CREATE INDEX idx_audit_entity ON audit_events(entity_type, entity_id, created_at);
CREATE INDEX idx_audit_type ON audit_events(event_type, created_at);

-- Historical records are corrected through linked entries or lifecycle state,
-- never by rewriting/deleting the original record.
CREATE TRIGGER star_transactions_no_update
BEFORE UPDATE ON star_transactions
BEGIN
  SELECT RAISE(ABORT, 'star-transactions-append-only');
END;

CREATE TRIGGER star_transactions_no_delete
BEFORE DELETE ON star_transactions
BEGIN
  SELECT RAISE(ABORT, 'star-transactions-append-only');
END;

CREATE TRIGGER audit_events_no_update
BEFORE UPDATE ON audit_events
BEGIN
  SELECT RAISE(ABORT, 'audit-events-append-only');
END;

CREATE TRIGGER audit_events_no_delete
BEFORE DELETE ON audit_events
BEGIN
  SELECT RAISE(ABORT, 'audit-events-append-only');
END;

CREATE TRIGGER task_completion_requests_no_delete
BEFORE DELETE ON task_completion_requests
BEGIN
  SELECT RAISE(ABORT, 'completion-history-cannot-be-deleted');
END;

CREATE TRIGGER reward_terms_no_delete
BEFORE DELETE ON reward_terms
BEGIN
  SELECT RAISE(ABORT, 'reward-term-history-cannot-be-deleted');
END;

CREATE TRIGGER reward_term_days_no_delete
BEFORE DELETE ON reward_term_days
BEGIN
  SELECT RAISE(ABORT, 'reward-term-day-history-cannot-be-deleted');
END;

CREATE TRIGGER reward_redemptions_no_delete
BEFORE DELETE ON reward_redemptions
BEGIN
  SELECT RAISE(ABORT, 'redemption-history-cannot-be-deleted');
END;

CREATE TRIGGER behavior_incidents_no_delete
BEFORE DELETE ON behavior_incidents
BEGIN
  SELECT RAISE(ABORT, 'incident-history-cannot-be-deleted');
END;

CREATE TRIGGER consequences_no_delete
BEFORE DELETE ON consequences
BEGIN
  SELECT RAISE(ABORT, 'consequence-history-cannot-be-deleted');
END;
