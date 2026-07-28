-- Separate task progress for the daily reward from task progress for the
-- larger reward while preserving the legacy behavior for existing records.
ALTER TABLE task_definitions ADD COLUMN default_larger_progress REAL NOT NULL DEFAULT 0
  CHECK (default_larger_progress >= 0);

UPDATE task_definitions
SET default_larger_progress = default_daily_progress;

ALTER TABLE task_assignments ADD COLUMN larger_progress_override REAL
  CHECK (larger_progress_override IS NULL OR larger_progress_override >= 0);

UPDATE task_assignments
SET larger_progress_override = daily_progress_override
WHERE daily_progress_override IS NOT NULL;

ALTER TABLE task_completion_requests ADD COLUMN effective_larger_progress REAL
  CHECK (effective_larger_progress IS NULL OR effective_larger_progress >= 0);

UPDATE task_completion_requests
SET effective_larger_progress = effective_daily_progress
WHERE effective_daily_progress IS NOT NULL;

ALTER TABLE reward_term_days ADD COLUMN task_larger_progress_value REAL NOT NULL DEFAULT 0
  CHECK (task_larger_progress_value >= 0);

-- Before this migration daily_progress_value served both reward scopes.
-- Copy it once so existing terms keep the same larger-reward progress.
UPDATE reward_term_days
SET task_larger_progress_value = daily_progress_value;

-- Profile switching is distinct from the existing "skip the picker when only
-- one child is enabled" launch convenience.
ALTER TABLE display_settings ADD COLUMN profile_switching_enabled INTEGER NOT NULL DEFAULT 1
  CHECK (profile_switching_enabled IN (0, 1));
