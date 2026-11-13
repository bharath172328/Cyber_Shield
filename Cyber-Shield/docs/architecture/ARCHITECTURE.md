# Architecture

```
┌────────────────────────┐   HTTPS/JSON    ┌───────────────────────────────────────────────┐
│ React 18 + Vite        │ ──────────────► │ C++17 API layer (cpp-httplib)                 │
│ Tailwind · Router      │   Bearer token  │  ApiServer → Auth / Security / Dashboard      │
│ Framer Motion · Recharts│ ◄────────────── │  controllers (validation, status codes, CORS) │
└────────────────────────┘                 └───────────────┬───────────────────────────────┘
                                                            │ plain C++ calls
                                   ┌────────────────────────▼───────────────────────────┐
                                   │ Security engine (no HTTP / DB knowledge)           │
                                   │ PasswordAnalyzer · URLAnalyzer · PortScanner       │
                                   │ FileHasher · FileAnalyzer · FileIntegrityMonitor   │
                                   │ ThreatDetector (score, stats, recommendations)     │
                                   └────────────────────────┬───────────────────────────┘
                     AuthenticationManager · SecurityLogger │
                                   ┌────────────────────────▼───────────────────────────┐
                                   │ DatabaseManager – the only SQLite access point     │
                                   │ (prepared statements, one mutex-guarded connection)│
                                   └────────────────────────────────────────────────────┘
```

## Backend layers
| Layer | Directory | Responsibility |
|---|---|---|
| API | `src/api` | Routing, JSON parsing, auth checks, HTTP status mapping, CORS |
| Security engine | `src/security` | Pure analysis logic; unit-tested without a server |
| Authentication | `src/authentication` | PBKDF2 hashing, sessions, login lockout |
| Logging | `src/logging` | Event → database **and** `logs/cybershield.log` |
| Database | `src/database` | SQLite wrapper with bound parameters only |
| Utils | `src/utils` | Config from environment, OpenSSL crypto helpers |

## Key design decisions
* **Parameterised SQL everywhere** – user input never becomes SQL text (verified with an injection probe in tests).
* **Sessions store only a SHA-256 of the token**, so a leaked database does not leak usable tokens.
* **Passwords are hashed with PBKDF2-HMAC-SHA256 (600 000 iterations, random 16-byte salt)**; analysed passwords are never stored or logged.
* **Least surprise for scanning:** loopback/private targets only unless an operator allow-lists a host; hard caps on ports, threads, timeouts and concurrency.
* **Static analysis only:** URLs are never fetched and files are never executed.
* **Frontend service layer:** components call `services/*`, which own all HTTP logic and error mapping.
* **Data isolation:** every query is scoped to the user; `scope=all` is admin-only.

## Configuration (environment variables)
`CS_PORT` (8080) · `CS_CORS_ORIGINS` (comma list, default the Vite dev origins) · `CS_DB_PATH` (`cybershield.db`) ·
`CS_LOG_PATH` (`logs/cybershield.log`) · `CS_SCAN_ALLOWED_HOSTS` (comma list) · `CS_SESSION_TTL_HOURS` (12) · `CS_SCHEMA_PATH`.
