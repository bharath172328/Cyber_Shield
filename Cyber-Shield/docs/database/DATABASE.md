# Database design (SQLite)

Schema file: `backend/database/schema.sql` (applied automatically on start-up; idempotent).

```
users 1───* sessions
users 1───* security_logs        (ON DELETE SET NULL – audit trail survives user removal)
users 1───* password_analysis    users 1───* url_analysis      users 1───* network_scans
users 1───* file_analysis        users 1───* file_integrity    users 1───* reports
```

| Table | Purpose | Notable constraints |
|---|---|---|
| `users` | Accounts | `username` UNIQUE NOCASE, length 3–32; `role` ∈ {admin,user}; `password_hash` (PBKDF2) |
| `sessions` | Login sessions | PK `token_hash` (SHA-256), FK → users, `expires_at` |
| `security_logs` | Audit log | `severity` ∈ {info,warning,error,critical}; indexes on `created_at`, `user_id`, `severity` |
| `password_analysis` | Score history (never the password) | `score` 0–100, `length` |
| `url_analysis` | URL results | `risk_score` 0–100, `indicators_json` |
| `network_scans` | Scan history | `CHECK port_start ≤ port_end ≤ 65535`, open/risky counts, `duration_ms` |
| `file_analysis` | File verdicts | `sha256`, `verdict`, `findings_json` |
| `file_integrity` | Trusted baselines | UNIQUE(`user_id`,`filename`); `status` ∈ {baseline,intact,modified} |
| `reports` | Saved report snapshots | `summary_json` |

All child tables use `FOREIGN KEY … ON DELETE CASCADE` (except logs), `PRAGMA foreign_keys=ON`, WAL journaling,
and `created_at` timestamps defaulting to UTC ISO-8601. Per-user + `created_at` indexes serve the dashboard's 7-day trend queries.
