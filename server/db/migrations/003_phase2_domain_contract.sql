ALTER TABLE task_assignments ADD COLUMN star_value_override INTEGER
  CHECK (star_value_override IS NULL OR star_value_override >= 0);
ALTER TABLE task_assignments ADD COLUMN daily_progress_override REAL
  CHECK (daily_progress_override IS NULL OR daily_progress_override >= 0);
ALTER TABLE task_assignments ADD COLUMN requires_approval_override INTEGER
  CHECK (requires_approval_override IS NULL OR requires_approval_override IN (0, 1));

ALTER TABLE task_completion_requests ADD COLUMN occurrence_date TEXT;
ALTER TABLE task_completion_requests ADD COLUMN effective_star_value INTEGER
  CHECK (effective_star_value IS NULL OR effective_star_value >= 0);
ALTER TABLE task_completion_requests ADD COLUMN effective_daily_progress REAL
  CHECK (effective_daily_progress IS NULL OR effective_daily_progress >= 0);
ALTER TABLE task_completion_requests ADD COLUMN rejected_by TEXT
  REFERENCES family_members(id) ON DELETE SET NULL;
ALTER TABLE task_completion_requests ADD COLUMN rejected_at TEXT;
ALTER TABLE task_completion_requests ADD COLUMN request_fingerprint TEXT;

ALTER TABLE star_transactions ADD COLUMN note TEXT;

ALTER TABLE reward_definitions ADD COLUMN milestone_threshold INTEGER
  CHECK (milestone_threshold IS NULL OR milestone_threshold > 0);

ALTER TABLE reward_terms ADD COLUMN qualification_mode TEXT NOT NULL DEFAULT 'successful_days'
  CHECK (qualification_mode IN ('successful_days', 'required_progress', 'either', 'both', 'parent_determination'));
ALTER TABLE reward_terms ADD COLUMN release_timing TEXT NOT NULL DEFAULT 'term_end'
  CHECK (release_timing IN ('threshold', 'term_end', 'manual'));
ALTER TABLE reward_terms ADD COLUMN household_timezone TEXT NOT NULL DEFAULT 'local';
ALTER TABLE reward_terms ADD COLUMN qualified_at TEXT;
ALTER TABLE reward_terms ADD COLUMN ready_at TEXT;
ALTER TABLE reward_terms ADD COLUMN finalized_at TEXT;
ALTER TABLE reward_terms ADD COLUMN reset_of TEXT REFERENCES reward_terms(id) ON DELETE RESTRICT;
ALTER TABLE reward_terms ADD COLUMN cycle_number INTEGER NOT NULL DEFAULT 1 CHECK (cycle_number > 0);

ALTER TABLE child_reward_goals ADD COLUMN term_id TEXT
  REFERENCES reward_terms(id) ON DELETE RESTRICT;
ALTER TABLE child_reward_goals ADD COLUMN readiness_basis TEXT;
ALTER TABLE child_reward_goals ADD COLUMN ready_at TEXT;
ALTER TABLE child_reward_goals ADD COLUMN request_key TEXT;

CREATE UNIQUE INDEX uq_child_reward_goals_request_key
  ON child_reward_goals(request_key) WHERE request_key IS NOT NULL;
CREATE UNIQUE INDEX uq_child_reward_goals_current
  ON child_reward_goals(child_id, goal_type)
  WHERE status IN ('selected', 'in_progress', 'ready', 'requested');

ALTER TABLE reward_redemptions ADD COLUMN goal_id TEXT
  REFERENCES child_reward_goals(id) ON DELETE SET NULL;
ALTER TABLE reward_redemptions ADD COLUMN declined_by TEXT
  REFERENCES family_members(id) ON DELETE SET NULL;
ALTER TABLE reward_redemptions ADD COLUMN declined_at TEXT;
ALTER TABLE reward_redemptions ADD COLUMN decline_reason TEXT;
ALTER TABLE reward_redemptions ADD COLUMN redeemed_at TEXT;
ALTER TABLE reward_redemptions ADD COLUMN completed_by TEXT
  REFERENCES family_members(id) ON DELETE SET NULL;
ALTER TABLE reward_redemptions ADD COLUMN cost_source TEXT NOT NULL DEFAULT 'definition'
  CHECK (cost_source IN ('definition', 'child_override', 'term_snapshot', 'milestone'));

ALTER TABLE domain_idempotency_keys ADD COLUMN request_hash TEXT NOT NULL DEFAULT '';
ALTER TABLE domain_idempotency_keys ADD COLUMN expires_at TEXT;

ALTER TABLE behavior_rules ADD COLUMN default_warning_behavior TEXT;
ALTER TABLE behavior_rules ADD COLUMN suggested_consequence TEXT;
ALTER TABLE behavior_rules ADD COLUMN suggested_recovery_action TEXT;

CREATE TABLE behavior_rule_children (
  rule_id TEXT NOT NULL REFERENCES behavior_rules(id) ON DELETE RESTRICT,
  child_id TEXT NOT NULL REFERENCES children(id) ON DELETE RESTRICT,
  active INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0, 1)),
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  PRIMARY KEY (rule_id, child_id)
);

ALTER TABLE behavior_incidents ADD COLUMN warning_level INTEGER NOT NULL DEFAULT 0
  CHECK (warning_level >= 0);

ALTER TABLE consequences ADD COLUMN child_id TEXT
  REFERENCES children(id) ON DELETE RESTRICT;
ALTER TABLE consequences ADD COLUMN rule_id TEXT
  REFERENCES behavior_rules(id) ON DELETE SET NULL;
ALTER TABLE consequences ADD COLUMN issued_by TEXT
  REFERENCES family_members(id) ON DELETE SET NULL;
ALTER TABLE consequences ADD COLUMN resolution TEXT;
ALTER TABLE consequences ADD COLUMN resolved_at TEXT;
ALTER TABLE consequences ADD COLUMN action_completed_at TEXT;

CREATE TABLE recovery_actions (
  id TEXT PRIMARY KEY,
  incident_id TEXT NOT NULL REFERENCES behavior_incidents(id) ON DELETE RESTRICT,
  consequence_id TEXT REFERENCES consequences(id) ON DELETE RESTRICT,
  rule_id TEXT REFERENCES behavior_rules(id) ON DELETE SET NULL,
  child_id TEXT NOT NULL REFERENCES children(id) ON DELETE RESTRICT,
  term_day_id TEXT REFERENCES reward_term_days(id) ON DELETE RESTRICT,
  required_action TEXT NOT NULL,
  deadline_at TEXT,
  credit_mode TEXT NOT NULL DEFAULT 'none'
    CHECK (credit_mode IN ('none', 'daily', 'term', 'daily_and_term')),
  daily_credit REAL NOT NULL DEFAULT 0 CHECK (daily_credit >= 0),
  term_credit REAL NOT NULL DEFAULT 0 CHECK (term_credit >= 0),
  parent_approval_required INTEGER NOT NULL DEFAULT 1
    CHECK (parent_approval_required IN (0, 1)),
  status TEXT NOT NULL DEFAULT 'pending'
    CHECK (status IN ('pending', 'requested', 'approved', 'completed', 'declined', 'expired', 'cancelled')),
  requested_at TEXT,
  approved_by TEXT REFERENCES family_members(id) ON DELETE SET NULL,
  approved_at TEXT,
  completed_at TEXT,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE INDEX idx_recovery_actions_child ON recovery_actions(child_id, status, deadline_at);
CREATE INDEX idx_recovery_actions_incident ON recovery_actions(incident_id, status);
CREATE INDEX idx_behavior_rule_children_child ON behavior_rule_children(child_id, active);
CREATE INDEX idx_domain_idempotency_expiry ON domain_idempotency_keys(expires_at);
CREATE INDEX idx_goals_term ON child_reward_goals(term_id, status);
CREATE INDEX idx_redemptions_goal ON reward_redemptions(goal_id, status);

CREATE TRIGGER recovery_actions_no_delete
BEFORE DELETE ON recovery_actions
BEGIN
  SELECT RAISE(ABORT, 'recovery-history-cannot-be-deleted');
END;
