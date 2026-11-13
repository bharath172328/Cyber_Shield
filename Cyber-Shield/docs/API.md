# Cyber Shield REST API

Base URL: `http://localhost:8080/api` · All bodies are JSON unless noted · Timestamps are UTC ISO-8601.

**Authentication:** every endpoint except `health`, `auth/register` and `auth/login` requires
`Authorization: Bearer <token>` (token from `POST /auth/login`, valid for `CS_SESSION_TTL_HOURS`, default 12).

**Errors** always look like `{"error": "message", "status": 400}`.

| Status | Meaning |
|-------:|---------|
| 200 / 201 | Success / created |
| 400 | Validation failed or malformed JSON |
| 401 | Missing, invalid or expired token; wrong credentials |
| 403 | Authenticated but not permitted (admin-only, blocked scan target) |
| 404 | Endpoint or resource not found |
| 409 | Conflict (username taken) |
| 413 | Upload larger than 25 MB |
| 429 | Too many failed logins (10-minute lock) / a scan is already running |
| 500 / 503 | Server error / database unavailable |

---
## System
### `GET /health`
No auth. Confirms the API **and** database are reachable.
```json
200 {"status":"ok","service":"cyber-shield","version":"1.0.0","database":{"status":"ok","engine":"sqlite 3.45.1","users":2}}
503 {"status":"degraded","database":{"status":"error"}}
```

## Authentication
### `POST /auth/register`
Body `{"username":"alice","email":"a@x.com","password":"Str0ng!Passphrase42"}` (`email` optional).
Username 3–32 chars `[A-Za-z0-9_.-]`; password 8–128 chars with analyzer score ≥ 50. **The first account becomes `admin`.**
`201 {"message":"Account created","user":{"id":1,"username":"alice","role":"admin"}}` · `400` weak/invalid · `409` duplicate.

### `POST /auth/login`
Body `{"username","password"}` → `200 {"token":"<64 hex>","expiresAt":"…","user":{…}}` · `401` bad credentials · `429` locked out.

### `GET /auth/me` → `200 {"user":{"id","username","role","email","createdAt","lastLogin"}}`
### `POST /auth/logout` → `200 {"message":"Logged out"}` (token is invalidated)
### `POST /auth/change-password`
Body `{"currentPassword","newPassword"}` → `200 {"message":"Password updated"}` · `401` wrong current · `400` weak new password.

## Password analyzer
### `POST /password/analyze`
Body `{"password":"Tr0ub4dor&3xyz"}` (1–256 chars). The password is **never stored or logged**.
```json
200 {"score":90,"strength":"Very Strong","message":"Password has excellent security","entropyBits":92.0,
     "findings":[],"checks":[{"name":"Length","passed":true,"detail":"14 characters (12+ recommended)"}, …]}
```
Strength bands: 0–29 Very Weak · 30–49 Weak · 50–69 Moderate · 70–84 Strong · 85–100 Very Strong.

## URL / phishing analyzer
### `POST /url/analyze`
Body `{"url":"http://paypal.com.secure-login.verify-account.xyz/signin"}`. Static analysis only – the URL is never fetched.
```json
200 {"url":"…","host":"…","scheme":"http","port":80,"path":"/signin","riskScore":92,"riskLevel":"Critical",
     "indicators":[{"id":"no_https","description":"…","severity":"medium","weight":15}, …],
     "recommendation":"Multiple strong phishing indicators. Do not visit; block the URL and report it."}
```
Risk levels: Low < 20 ≤ Medium < 45 ≤ High < 70 ≤ Critical. `400` for non-http(s) schemes or unparsable input.

## Network scanner
### `POST /network/scan`
Body `{"host":"127.0.0.1","startPort":1,"endPort":1024,"authorized":true}`.
Safety rules: `authorized` must be `true`; host must resolve to loopback / RFC1918 (or be in `CS_SCAN_ALLOWED_HOSTS`);
max 1024 ports; one scan at a time; 64 threads, 400 ms timeout per port; no banner grabbing or payloads.
```json
200 {"host":"127.0.0.1","resolvedIp":"127.0.0.1","startPort":6375,"endPort":6380,"durationMs":3,
     "openCount":1,"closedCount":5,"filteredCount":0,"riskyOpenCount":1,
     "ports":[{"port":6379,"state":"open","service":"Redis","risk":"high","note":"…"}, …]}
```
`400` invalid input/too many ports · `403` target not permitted · `429` scan already running.

## File security
Uploads use `multipart/form-data` with a `file` field (max 25 MB). Files are analysed in memory and never executed.

### `POST /file/hash`
```json
200 {"filename":"note.txt","size":6,"hashes":{"sha256":"…","sha1":"…","md5":"…"},"md5Note":"…",
     "fileType":"Plain text","analysisResult":"Clean","riskScore":0}
```
### `POST /file/analyze`
```json
200 {"filename":"invoice.pdf.exe","size":103,"sha256":"…","fileType":"Windows executable (PE)","extension":"exe",
     "entropy":0.4,"riskScore":65,"verdict":"High Risk",
     "findings":[{"id":"double_extension","description":"…","severity":"high","weight":40}, …]}
```
Verdicts: Clean (<15) · Suspicious (15–44) · High Risk (≥45). Detects disguised executables, double extensions,
packed content (entropy), macro/PDF-action indicators and the harmless EICAR test signature.

### File integrity
| Method & URL | Description |
|---|---|
| `GET /file/integrity` | `{"items":[{id,filename,baseline_hash,current_hash,status,created_at,last_checked}]}` |
| `POST /file/integrity/baseline` (`file`) | Store/replace trusted hash → `201 {"filename","baselineHash","status":"baseline"}` |
| `POST /file/integrity/verify` (`file`, optional `expectedHash`) | `200 {"status":"intact\|modified\|no_baseline","match","baselineHash","currentHash","source":"stored\|provided"}` |
| `DELETE /file/integrity/{id}` | `200 {"message":"Baseline deleted"}` · `404` (also for other users' baselines) |

## Dashboard, logs, reports
### `GET /dashboard[?scope=all]`
Returns `securityScore`, `securityLabel`, `threatCount`, `criticalAlerts`, `warningAlerts`, `passwordStats`, `urlStats`,
`networkStats`, `fileStats`, 7-day `trend[]` and `recentEvents[]`. `scope=all` (all users) requires admin → otherwise `403`.

### `GET /logs`
Query: `page` (1), `pageSize` (20, max 100), `severity` (info|warning|error|critical), `type`, `q` (text search), `scope=all` (admin).
`200 {"items":[{id,event_type,severity,description,ip_address,created_at,username}],"total":57,"page":1,"pageSize":20}`

### `GET /reports` → `{"items":[{"id","title","createdAt","summary":{…}}]}`
### `POST /reports/generate` — body `{"title":"Weekly"}` (optional) → `201 {"id","title","summary":{securityScore,…,recommendations:[…]}}`

## Administration
### `GET /admin/users` (admin only) → `{"items":[{id,username,email,role,created_at,last_login}]}` · `403` for non-admins.

## Quick example
```bash
TOKEN=$(curl -s -X POST localhost:8080/api/auth/login -H 'Content-Type: application/json' \
        -d '{"username":"alice","password":"Str0ng!Passphrase42"}' | python3 -c 'import sys,json;print(json.load(sys.stdin)["token"])')
curl -X POST localhost:8080/api/url/analyze -H "Authorization: Bearer $TOKEN" -H 'Content-Type: application/json' \
     -d '{"url":"http://192.168.1.10/login"}'
```
