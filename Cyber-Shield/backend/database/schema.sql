PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS users (
  id            INTEGER PRIMARY KEY AUTOINCREMENT,
  username      TEXT NOT NULL UNIQUE COLLATE NOCASE CHECK (length(username) BETWEEN 3 AND 32),
  email         TEXT,
  password_hash TEXT NOT NULL,
  role          TEXT NOT NULL DEFAULT 'user' CHECK (role IN ('admin','user')),
  created_at    TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now')),
  last_login    TEXT
);

CREATE TABLE IF NOT EXISTS sessions (
  token_hash TEXT PRIMARY KEY,
  user_id    INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now')),
  expires_at TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_sessions_user ON sessions(user_id);

CREATE TABLE IF NOT EXISTS security_logs (
  id          INTEGER PRIMARY KEY AUTOINCREMENT,
  user_id     INTEGER REFERENCES users(id) ON DELETE SET NULL,
  event_type  TEXT NOT NULL,
  severity    TEXT NOT NULL CHECK (severity IN ('info','warning','error','critical')),
  description TEXT NOT NULL,
  ip_address  TEXT,
  created_at  TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now'))
);
CREATE INDEX IF NOT EXISTS idx_logs_created  ON security_logs(created_at);
CREATE INDEX IF NOT EXISTS idx_logs_user     ON security_logs(user_id);
CREATE INDEX IF NOT EXISTS idx_logs_severity ON security_logs(severity);

CREATE TABLE IF NOT EXISTS password_analysis (
  id         INTEGER PRIMARY KEY AUTOINCREMENT,
  user_id    INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  score      INTEGER NOT NULL CHECK (score BETWEEN 0 AND 100),
  strength   TEXT NOT NULL,
  length     INTEGER NOT NULL,
  created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now'))
);
CREATE INDEX IF NOT EXISTS idx_pw_user ON password_analysis(user_id, created_at);

CREATE TABLE IF NOT EXISTS url_analysis (
  id              INTEGER PRIMARY KEY AUTOINCREMENT,
  user_id         INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  url             TEXT NOT NULL,
  risk_score      INTEGER NOT NULL CHECK (risk_score BETWEEN 0 AND 100),
  risk_level      TEXT NOT NULL,
  indicators_json TEXT NOT NULL DEFAULT '[]',
  created_at      TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now'))
);
CREATE INDEX IF NOT EXISTS idx_url_user ON url_analysis(user_id, created_at);

CREATE TABLE IF NOT EXISTS network_scans (
  id               INTEGER PRIMARY KEY AUTOINCREMENT,
  user_id          INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  host             TEXT NOT NULL,
  port_start       INTEGER NOT NULL,
  port_end         INTEGER NOT NULL,
  open_count       INTEGER NOT NULL DEFAULT 0,
  risky_open_count INTEGER NOT NULL DEFAULT 0,
  open_ports_json  TEXT NOT NULL DEFAULT '[]',
  duration_ms      INTEGER NOT NULL DEFAULT 0,
  created_at       TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now')),
  CHECK (port_start BETWEEN 1 AND 65535 AND port_end BETWEEN port_start AND 65535)
);
CREATE INDEX IF NOT EXISTS idx_scan_user ON network_scans(user_id, created_at);

CREATE TABLE IF NOT EXISTS file_analysis (
  id            INTEGER PRIMARY KEY AUTOINCREMENT,
  user_id       INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  filename      TEXT NOT NULL,
  size          INTEGER NOT NULL,
  sha256        TEXT NOT NULL,
  file_type     TEXT NOT NULL,
  risk_score    INTEGER NOT NULL CHECK (risk_score BETWEEN 0 AND 100),
  verdict       TEXT NOT NULL,
  findings_json TEXT NOT NULL DEFAULT '[]',
  created_at    TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now'))
);
CREATE INDEX IF NOT EXISTS idx_file_user ON file_analysis(user_id, created_at);

CREATE TABLE IF NOT EXISTS file_integrity (
  id            INTEGER PRIMARY KEY AUTOINCREMENT,
  user_id       INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  filename      TEXT NOT NULL,
  baseline_hash TEXT NOT NULL,
  current_hash  TEXT,
  status        TEXT NOT NULL DEFAULT 'baseline' CHECK (status IN ('baseline','intact','modified')),
  created_at    TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now')),
  last_checked  TEXT,
  UNIQUE (user_id, filename)
);

CREATE TABLE IF NOT EXISTS reports (
  id           INTEGER PRIMARY KEY AUTOINCREMENT,
  user_id      INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  title        TEXT NOT NULL,
  summary_json TEXT NOT NULL,
  created_at   TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now'))
);
CREATE INDEX IF NOT EXISTS idx_reports_user ON reports(user_id, created_at);