# 🛡️ Cyber Shield — Cybersecurity Threat Detection & Monitoring System

An MCA-level security platform with a **C++17 security engine and REST API** and a **React dashboard**.
It analyses passwords, URLs, files and networks, records every event in an audit log, and rolls the results up into a live security score.

> Screenshots: _add images to `docs/screenshots/` and link them here_
> `![Dashboard](docs/screenshots/dashboard.png)`

## Features
| Module | What it does |
|---|---|
| **Password Analyzer** | Length, character classes, repeats, sequences, common patterns, entropy → 0-100 score. Passwords are never stored. |
| **URL / Phishing Analyzer** | HTTPS use, IP hosts, subdomain depth, look-alike & brand impersonation, keywords, shorteners, encoding → risk score & advice. Static only; never fetches the URL. |
| **Network Scanner** | Authorised TCP connect scan with service names and risk notes. Loopback/private targets only unless allow-listed; ≤1024 ports; one scan at a time. |
| **File Security** | SHA-256 / SHA-1 / MD5, magic-byte type detection, disguised-executable, double-extension, entropy and macro/script indicators. Never executes files. |
| **File Integrity** | Store a trusted baseline hash and detect later modification. |
| **Security Logs** | Timestamp, event type, severity, description, user, IP; search, filter, pagination. |
| **Dashboard & Reports** | Security score, threat counts, 7-day trends, charts, saved reports with recommendations. |
| **Auth & Roles** | PBKDF2 password hashing, expiring sessions, login lockout, first user = admin, admin console. |

## Architecture
`React (Vite/Tailwind) → REST/JSON → C++17 API (cpp-httplib) → Security engine → SQLite`
See [docs/architecture/ARCHITECTURE.md](docs/architecture/ARCHITECTURE.md), [docs/database/DATABASE.md](docs/database/DATABASE.md) and [docs/API.md](docs/API.md).

## Technology stack
C++17 · CMake · cpp-httplib · nlohmann/json · OpenSSL (hashing, PBKDF2) · SQLite3 —
React 18 · Vite · Tailwind CSS 3 · React Router 6 · Axios · Framer Motion · Recharts · lucide-react.

## Folder structure
```
Cyber-Shield/
├── backend/            CMakeLists.txt, include/, src/{api,security,authentication,database,logging,utils}, database/schema.sql, tests/
├── frontend/           src/{components,pages,layouts,hooks,services,context,utils}, vite.config.js
├── docs/               API.md, architecture/, database/
├── tests/              api_smoke.sh (end-to-end API test)
└── README.md
```

## Installation & running
**Prerequisites:** g++ ≥ 9 (or clang), CMake ≥ 3.16, Git, OpenSSL and SQLite3 dev packages, Node.js ≥ 18.
```bash
# Ubuntu/Debian
sudo apt install build-essential cmake git libssl-dev libsqlite3-dev
```

### 1 · Backend (C++)
```bash
cd backend
cmake -S . -B build          # downloads cpp-httplib & nlohmann/json (needs internet the first time)
cmake --build build -j
./build/cs_tests             # unit tests
./build/cybershield          # API on http://localhost:8080
```
### 2 · Database
Nothing to install: SQLite creates `cybershield.db` in the working directory and applies `backend/database/schema.sql` on start-up.
Point elsewhere with `CS_DB_PATH=/path/to/file.db`.

### 3 · Frontend
**Option A - development (hot reload):**
```bash
cd frontend
npm install
npm run dev                  # http://localhost:5173  (/api is proxied to the backend on :8080)
```
**Option B - single port (dashboard served by the C++ backend):**
```bash
cd frontend && npm install && npm run build     # creates frontend/dist
cd ../backend && ./build/cybershield            # open http://localhost:8080
```
Open the site, **register** (the first account becomes admin) and explore.
If you open `http://localhost:8080` before building the frontend you now get a help page instead of "Endpoint not found".

### Configuration (environment variables, backend)
`CS_PORT` · `CS_FRONTEND_DIR` · `CS_CORS_ORIGINS` · `CS_DB_PATH` · `CS_LOG_PATH` · `CS_SCAN_ALLOWED_HOSTS` · `CS_SESSION_TTL_HOURS`.

## Testing
| Command | Covers |
|---|---|
| `backend/build/cs_tests` | Hash vectors, PBKDF2, password/URL/file analyzers, scanner safety limits |
| `tests/api_smoke.sh [url]` | 35 end-to-end API checks (auth, validation, RBAC, scan limits, integrity, reports). Use a fresh DB. |
| `cd frontend && npm test` | 18 jsdom tests that drive the real React app against a running backend (needs a registered user; override with `CS_TEST_USER` / `CS_TEST_PASS`) |

## API endpoints (summary)
`GET /api/health` · `POST /api/auth/{register,login,logout,change-password}` · `GET /api/auth/me` ·
`POST /api/password/analyze` · `POST /api/url/analyze` · `POST /api/network/scan` · `POST /api/file/{hash,analyze}` ·
`GET|POST|DELETE /api/file/integrity…` · `GET /api/logs` · `GET /api/dashboard` · `GET /api/reports` · `POST /api/reports/generate` · `GET /api/admin/users`.
Full details: [docs/API.md](docs/API.md).

## Security notes & limitations
* Ships as **HTTP for local development**. Put it behind a TLS-terminating reverse proxy (nginx/Caddy) for any real deployment — passwords are sent to the API in request bodies.
* Session tokens are kept in the browser's `localStorage`; a production hardening step would be `HttpOnly` cookies + CSRF protection.
* Rate limiting exists for logins only. Registration is open (first user = admin) — disable or invite-gate it in production.
* Heuristic analyzers (URL, file) are educational detectors, **not** a substitute for antivirus or threat-intelligence feeds.
* The scanner only reports what a TCP connect reveals; it does not banner-grab, fingerprint or exploit.

## Future improvements
TLS in-process, HttpOnly cookie sessions, TOTP two-factor auth, threat-intelligence lookups (e.g. URL blocklists), scheduled integrity monitoring, PDF export of reports, MySQL/PostgreSQL backend, Docker Compose, role management UI, code-split frontend bundle.
