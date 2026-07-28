CREATE TABLE households (
  id TEXT PRIMARY KEY,
  name TEXT NOT NULL,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

INSERT INTO households (id, name) VALUES ('default', 'Family Hub household');

CREATE TABLE child_households (
  household_id TEXT NOT NULL REFERENCES households(id) ON DELETE RESTRICT,
  child_id TEXT NOT NULL UNIQUE REFERENCES children(id) ON DELETE RESTRICT,
  active INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0, 1)),
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  PRIMARY KEY (household_id, child_id)
);

INSERT INTO child_households (household_id, child_id)
SELECT 'default', id FROM children;

CREATE TRIGGER children_assign_default_household
AFTER INSERT ON children
BEGIN
  INSERT OR IGNORE INTO child_households (household_id, child_id)
  VALUES ('default', NEW.id);
END;

ALTER TABLE reward_term_days ADD COLUMN behavior_daily_progress_value REAL NOT NULL DEFAULT 0
  CHECK (behavior_daily_progress_value >= 0);
ALTER TABLE reward_term_days ADD COLUMN behavior_larger_progress_value REAL NOT NULL DEFAULT 0
  CHECK (behavior_larger_progress_value >= 0);

CREATE TABLE behavior_definitions (
  id TEXT PRIMARY KEY,
  household_id TEXT NOT NULL REFERENCES households(id) ON DELETE RESTRICT,
  name TEXT NOT NULL,
  description TEXT NOT NULL DEFAULT '',
  preset_group TEXT,
  category TEXT NOT NULL CHECK (category IN ('positive', 'developing', 'recovery', 'challenging', 'safety')),
  classification_default TEXT NOT NULL CHECK (classification_default IN (
    'positive_independent', 'positive_prompted', 'skill_attempt', 'skill_progress',
    'recovery', 'challenging', 'safety'
  )),
  icon TEXT NOT NULL DEFAULT '',
  image_asset_id TEXT REFERENCES media_assets(id) ON DELETE SET NULL,
  active INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0, 1)),
  default_star_value INTEGER NOT NULL DEFAULT 0 CHECK (default_star_value >= 0),
  reward_eligible INTEGER NOT NULL DEFAULT 0 CHECK (reward_eligible IN (0, 1)),
  reward_classifications_json TEXT NOT NULL DEFAULT '["positive_independent"]',
  supports_prompt_level INTEGER NOT NULL DEFAULT 0 CHECK (supports_prompt_level IN (0, 1)),
  supports_duration INTEGER NOT NULL DEFAULT 0 CHECK (supports_duration IN (0, 1)),
  supports_intensity INTEGER NOT NULL DEFAULT 0 CHECK (supports_intensity IN (0, 1)),
  can_create_incident INTEGER NOT NULL DEFAULT 0 CHECK (can_create_incident IN (0, 1)),
  child_visible INTEGER NOT NULL DEFAULT 0 CHECK (child_visible IN (0, 1)),
  parent_only INTEGER NOT NULL DEFAULT 1 CHECK (parent_only IN (0, 1)),
  goal_eligible INTEGER NOT NULL DEFAULT 0 CHECK (goal_eligible IN (0, 1)),
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now')),
  UNIQUE (household_id, id)
);

CREATE TABLE behavior_definition_children (
  household_id TEXT NOT NULL,
  behavior_definition_id TEXT NOT NULL,
  child_id TEXT NOT NULL,
  active INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0, 1)),
  reward_eligible_override INTEGER CHECK (reward_eligible_override IS NULL OR reward_eligible_override IN (0, 1)),
  star_value_override INTEGER CHECK (star_value_override IS NULL OR star_value_override >= 0),
  child_visible_override INTEGER CHECK (child_visible_override IS NULL OR child_visible_override IN (0, 1)),
  goal_enabled INTEGER NOT NULL DEFAULT 0 CHECK (goal_enabled IN (0, 1)),
  goal_target REAL CHECK (goal_target IS NULL OR goal_target > 0),
  display_order INTEGER NOT NULL DEFAULT 0,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now')),
  PRIMARY KEY (household_id, behavior_definition_id, child_id),
  FOREIGN KEY (household_id, behavior_definition_id)
    REFERENCES behavior_definitions(household_id, id) ON DELETE RESTRICT,
  FOREIGN KEY (household_id, child_id)
    REFERENCES child_households(household_id, child_id) ON DELETE RESTRICT
);

CREATE TABLE behavior_reward_links (
  household_id TEXT NOT NULL,
  behavior_definition_id TEXT NOT NULL,
  child_id TEXT NOT NULL,
  reward_scope TEXT NOT NULL CHECK (reward_scope IN ('daily', 'larger')),
  contribution_value REAL NOT NULL DEFAULT 1 CHECK (contribution_value > 0),
  active INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0, 1)),
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now')),
  PRIMARY KEY (household_id, behavior_definition_id, child_id, reward_scope),
  FOREIGN KEY (household_id, behavior_definition_id, child_id)
    REFERENCES behavior_definition_children(household_id, behavior_definition_id, child_id) ON DELETE RESTRICT
);

CREATE TABLE behavior_observations (
  id TEXT PRIMARY KEY,
  household_id TEXT NOT NULL,
  child_id TEXT NOT NULL,
  behavior_definition_id TEXT NOT NULL,
  classification TEXT NOT NULL CHECK (classification IN (
    'positive_independent', 'positive_prompted', 'skill_attempt', 'skill_progress',
    'recovery', 'challenging', 'safety'
  )),
  occurred_at TEXT NOT NULL,
  context TEXT,
  antecedent TEXT,
  observable_behavior TEXT,
  prompt_level TEXT CHECK (prompt_level IS NULL OR prompt_level IN (
    'independent', 'gestural', 'verbal', 'modeled', 'partial_physical',
    'full_assistance', 'not_applicable'
  )),
  intensity INTEGER CHECK (intensity IS NULL OR intensity BETWEEN 1 AND 5),
  duration_seconds INTEGER CHECK (duration_seconds IS NULL OR duration_seconds >= 0),
  response_type TEXT,
  response_description TEXT,
  outcome TEXT CHECK (outcome IS NULL OR outcome IN (
    'successful', 'partially_successful', 'resolved_immediately',
    'resolved_after_prompt', 'resolved_after_support', 'recovered',
    'continued', 'escalated', 'unknown'
  )),
  effective INTEGER CHECK (effective IS NULL OR effective IN (0, 1)),
  star_value_awarded INTEGER NOT NULL DEFAULT 0 CHECK (star_value_awarded >= 0),
  goal_contribution REAL NOT NULL DEFAULT 0 CHECK (goal_contribution >= 0),
  incident_id TEXT REFERENCES behavior_incidents(id) ON DELETE RESTRICT,
  recorded_by TEXT,
  source TEXT NOT NULL CHECK (source IN ('web', 'child_firmware', 'system', 'import')),
  idempotency_key TEXT,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  updated_at TEXT NOT NULL DEFAULT (datetime('now')),
  FOREIGN KEY (household_id, child_id)
    REFERENCES child_households(household_id, child_id) ON DELETE RESTRICT,
  FOREIGN KEY (household_id, behavior_definition_id)
    REFERENCES behavior_definitions(household_id, id) ON DELETE RESTRICT,
  UNIQUE (household_id, id),
  UNIQUE (household_id, child_id, id)
);

CREATE TABLE behavior_observation_amendments (
  id TEXT PRIMARY KEY,
  household_id TEXT NOT NULL,
  observation_id TEXT NOT NULL,
  sequence_no INTEGER NOT NULL CHECK (sequence_no > 0),
  amendment_type TEXT NOT NULL CHECK (amendment_type IN ('correction', 'annotation', 'archive')),
  reason TEXT NOT NULL,
  patch_json TEXT NOT NULL DEFAULT '{}',
  recorded_by TEXT NOT NULL,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  UNIQUE (household_id, observation_id, sequence_no),
  FOREIGN KEY (household_id, observation_id)
    REFERENCES behavior_observations(household_id, id) ON DELETE RESTRICT
);

CREATE TABLE behavior_observation_contributions (
  id TEXT PRIMARY KEY,
  household_id TEXT NOT NULL,
  observation_id TEXT NOT NULL UNIQUE,
  star_transaction_id TEXT UNIQUE REFERENCES star_transactions(id) ON DELETE RESTRICT,
  star_value INTEGER NOT NULL DEFAULT 0 CHECK (star_value >= 0),
  goal_contribution REAL NOT NULL DEFAULT 0 CHECK (goal_contribution >= 0),
  reward_eligible INTEGER NOT NULL DEFAULT 0 CHECK (reward_eligible IN (0, 1)),
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  FOREIGN KEY (household_id, observation_id)
    REFERENCES behavior_observations(household_id, id) ON DELETE RESTRICT
);

CREATE TABLE behavior_observation_reward_contributions (
  id TEXT PRIMARY KEY,
  household_id TEXT NOT NULL,
  observation_id TEXT NOT NULL,
  reward_scope TEXT NOT NULL CHECK (reward_scope IN ('daily', 'larger')),
  reward_term_id TEXT REFERENCES reward_terms(id) ON DELETE RESTRICT,
  reward_term_day_id TEXT REFERENCES reward_term_days(id) ON DELETE RESTRICT,
  contribution_value REAL NOT NULL CHECK (contribution_value > 0),
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  UNIQUE (observation_id, reward_scope),
  FOREIGN KEY (household_id, observation_id)
    REFERENCES behavior_observations(household_id, id) ON DELETE RESTRICT
);

CREATE TABLE behavior_action_requests (
  id TEXT PRIMARY KEY,
  household_id TEXT NOT NULL,
  child_id TEXT NOT NULL,
  behavior_definition_id TEXT NOT NULL,
  classification TEXT NOT NULL CHECK (classification IN (
    'positive_independent', 'positive_prompted', 'skill_attempt', 'skill_progress', 'recovery'
  )),
  request_key TEXT NOT NULL,
  status TEXT NOT NULL DEFAULT 'pending' CHECK (status IN ('pending', 'approved', 'rejected', 'cancelled')),
  requested_at TEXT NOT NULL DEFAULT (datetime('now')),
  decided_at TEXT,
  decided_by TEXT,
  rejection_reason TEXT,
  observation_id TEXT UNIQUE,
  updated_at TEXT NOT NULL DEFAULT (datetime('now')),
  UNIQUE (household_id, child_id, request_key),
  FOREIGN KEY (household_id, child_id)
    REFERENCES child_households(household_id, child_id) ON DELETE RESTRICT,
  FOREIGN KEY (household_id, behavior_definition_id, child_id)
    REFERENCES behavior_definition_children(household_id, behavior_definition_id, child_id) ON DELETE RESTRICT,
  FOREIGN KEY (household_id, child_id, observation_id)
    REFERENCES behavior_observations(household_id, child_id, id) ON DELETE RESTRICT
);

CREATE INDEX idx_child_households_household ON child_households(household_id, active, child_id);
CREATE INDEX idx_behavior_definitions_household ON behavior_definitions(household_id, active, category, name);
CREATE INDEX idx_behavior_definition_children_child ON behavior_definition_children(household_id, child_id, active, display_order);
CREATE INDEX idx_behavior_reward_links_child ON behavior_reward_links(household_id, child_id, active, reward_scope);
CREATE INDEX idx_behavior_observations_household ON behavior_observations(household_id, occurred_at);
CREATE INDEX idx_behavior_observations_child ON behavior_observations(household_id, child_id, occurred_at DESC);
CREATE INDEX idx_behavior_observations_definition ON behavior_observations(household_id, behavior_definition_id, occurred_at DESC);
CREATE INDEX idx_behavior_observations_classification ON behavior_observations(household_id, classification, occurred_at DESC);
CREATE INDEX idx_behavior_observations_source ON behavior_observations(household_id, source, occurred_at DESC);
CREATE INDEX idx_behavior_observations_incident ON behavior_observations(incident_id) WHERE incident_id IS NOT NULL;
CREATE UNIQUE INDEX uq_behavior_observation_retry
  ON behavior_observations(household_id, source, idempotency_key)
  WHERE idempotency_key IS NOT NULL;
CREATE INDEX idx_behavior_amendments_observation ON behavior_observation_amendments(household_id, observation_id, sequence_no);
CREATE INDEX idx_behavior_reward_contributions_observation ON behavior_observation_reward_contributions(household_id, observation_id);
CREATE INDEX idx_behavior_action_requests_queue ON behavior_action_requests(household_id, status, requested_at);
CREATE INDEX idx_behavior_action_requests_child ON behavior_action_requests(household_id, child_id, status, requested_at);

CREATE TRIGGER behavior_observations_validate_incident
BEFORE INSERT ON behavior_observations
WHEN NEW.incident_id IS NOT NULL
BEGIN
  SELECT CASE WHEN NOT EXISTS (
    SELECT 1
    FROM behavior_incidents incident
    JOIN child_households scope ON scope.child_id = incident.child_id
    WHERE incident.id = NEW.incident_id
      AND incident.child_id = NEW.child_id
      AND scope.household_id = NEW.household_id
  ) THEN RAISE(ABORT, 'observation-incident-scope-mismatch') END;
END;

CREATE TRIGGER behavior_observations_no_update
BEFORE UPDATE ON behavior_observations
BEGIN
  SELECT RAISE(ABORT, 'behavior-observations-append-only');
END;

CREATE TRIGGER behavior_observations_no_delete
BEFORE DELETE ON behavior_observations
BEGIN
  SELECT RAISE(ABORT, 'behavior-observations-append-only');
END;

CREATE TRIGGER behavior_observation_amendments_no_update
BEFORE UPDATE ON behavior_observation_amendments
BEGIN
  SELECT RAISE(ABORT, 'behavior-observation-amendments-append-only');
END;

CREATE TRIGGER behavior_observation_amendments_no_delete
BEFORE DELETE ON behavior_observation_amendments
BEGIN
  SELECT RAISE(ABORT, 'behavior-observation-amendments-append-only');
END;

CREATE TRIGGER behavior_observation_contributions_no_update
BEFORE UPDATE ON behavior_observation_contributions
BEGIN
  SELECT RAISE(ABORT, 'behavior-observation-contributions-append-only');
END;

CREATE TRIGGER behavior_observation_contributions_validate_transaction
BEFORE INSERT ON behavior_observation_contributions
WHEN NEW.star_transaction_id IS NOT NULL
BEGIN
  SELECT CASE WHEN NOT EXISTS (
    SELECT 1 FROM behavior_observations observation
    JOIN star_transactions transaction_row
      ON transaction_row.id = NEW.star_transaction_id
     AND transaction_row.child_id = observation.child_id
    WHERE observation.household_id = NEW.household_id
      AND observation.id = NEW.observation_id
  ) THEN RAISE(ABORT, 'observation-star-transaction-scope-mismatch') END;
END;

CREATE TRIGGER behavior_observation_contributions_no_delete
BEFORE DELETE ON behavior_observation_contributions
BEGIN
  SELECT RAISE(ABORT, 'behavior-observation-contributions-append-only');
END;

CREATE TRIGGER behavior_observation_reward_contributions_no_update
BEFORE UPDATE ON behavior_observation_reward_contributions
BEGIN
  SELECT RAISE(ABORT, 'behavior-observation-reward-contributions-append-only');
END;

CREATE TRIGGER behavior_observation_reward_contributions_validate_term
BEFORE INSERT ON behavior_observation_reward_contributions
WHEN NEW.reward_term_id IS NOT NULL OR NEW.reward_term_day_id IS NOT NULL
BEGIN
  SELECT CASE WHEN NOT EXISTS (
    SELECT 1 FROM behavior_observations observation
    JOIN reward_terms term
      ON term.id = NEW.reward_term_id AND term.child_id = observation.child_id
    LEFT JOIN reward_term_days term_day
      ON term_day.id = NEW.reward_term_day_id AND term_day.term_id = term.id
    WHERE observation.household_id = NEW.household_id
      AND observation.id = NEW.observation_id
      AND (NEW.reward_term_day_id IS NULL OR term_day.id IS NOT NULL)
  ) THEN RAISE(ABORT, 'observation-reward-term-scope-mismatch') END;
END;

CREATE TRIGGER behavior_observation_reward_contributions_no_delete
BEFORE DELETE ON behavior_observation_reward_contributions
BEGIN
  SELECT RAISE(ABORT, 'behavior-observation-reward-contributions-append-only');
END;

CREATE TRIGGER behavior_action_requests_no_delete
BEFORE DELETE ON behavior_action_requests
BEGIN
  SELECT RAISE(ABORT, 'behavior-action-request-history-cannot-be-deleted');
END;
