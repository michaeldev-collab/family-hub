CREATE TABLE parent_credentials (
  id INTEGER PRIMARY KEY CHECK (id = 1),
  algorithm TEXT NOT NULL CHECK (algorithm = 'scrypt'),
  scrypt_n INTEGER NOT NULL,
  scrypt_r INTEGER NOT NULL,
  scrypt_p INTEGER NOT NULL,
  salt BLOB NOT NULL,
  pin_hash BLOB NOT NULL,
  pin_version INTEGER NOT NULL DEFAULT 1 CHECK (pin_version > 0),
  failed_attempts INTEGER NOT NULL DEFAULT 0 CHECK (failed_attempts >= 0),
  locked_until_ms INTEGER NOT NULL DEFAULT 0 CHECK (locked_until_ms >= 0),
  created_at_ms INTEGER NOT NULL,
  updated_at_ms INTEGER NOT NULL
);

CREATE TABLE parent_sessions (
  id TEXT PRIMARY KEY,
  token_hash BLOB NOT NULL UNIQUE,
  csrf_hash BLOB NOT NULL,
  pin_version INTEGER NOT NULL,
  created_at_ms INTEGER NOT NULL,
  last_seen_at_ms INTEGER NOT NULL,
  idle_expires_at_ms INTEGER NOT NULL,
  absolute_expires_at_ms INTEGER NOT NULL,
  revoked_at_ms INTEGER,
  CHECK (idle_expires_at_ms <= absolute_expires_at_ms)
);

CREATE TABLE auth_rate_limits (
  scope TEXT NOT NULL,
  key_hash BLOB NOT NULL,
  failures INTEGER NOT NULL DEFAULT 0 CHECK (failures >= 0),
  blocked_until_ms INTEGER NOT NULL DEFAULT 0 CHECK (blocked_until_ms >= 0),
  last_failure_at_ms INTEGER NOT NULL,
  PRIMARY KEY (scope, key_hash)
);

CREATE TABLE auth_system_secrets (
  name TEXT PRIMARY KEY,
  secret BLOB NOT NULL,
  created_at_ms INTEGER NOT NULL
);

CREATE INDEX idx_parent_sessions_expiry
  ON parent_sessions(absolute_expires_at_ms, idle_expires_at_ms);
CREATE INDEX idx_parent_sessions_active
  ON parent_sessions(revoked_at_ms, absolute_expires_at_ms);
CREATE INDEX idx_auth_rate_limits_cleanup
  ON auth_rate_limits(last_failure_at_ms);
