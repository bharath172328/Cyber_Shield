#!/usr/bin/env bash
# End-to-end API test. Usage: tests/api_smoke.sh [base_url]
# Run against a FRESH database (first registered user becomes admin), e.g.:
#   CS_DB_PATH=/tmp/cs_test.db ./backend/build/cybershield &
set -u
B="${1:-http://localhost:8080}/api"; J='Content-Type: application/json'
pass=0; fail=0
check() { # name expected_status actual_status
  if [ "$2" = "$3" ]; then pass=$((pass+1)); echo "  ok   $1"; else fail=$((fail+1)); echo "  FAIL $1 (expected $2, got $3)"; fi; }
code() { curl -s -o /dev/null -w '%{http_code}' "$@"; }
jget() { python3 -c "import sys,json;d=json.load(sys.stdin);print(eval(sys.argv[1]))" "$1"; }
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT

check "health"                       200 "$(code $B/health)"
check "protected route w/o token"    401 "$(code $B/dashboard)"
check "register weak password"       400 "$(code -X POST $B/auth/register -H "$J" -d '{"username":"admin1","password":"password1"}')"
check "register admin"               201 "$(code -X POST $B/auth/register -H "$J" -d '{"username":"admin1","password":"Str0ng!Passphrase42"}')"
check "register second user"         201 "$(code -X POST $B/auth/register -H "$J" -d '{"username":"user1","password":"Another#Str0ng99pw"}')"
check "duplicate username"           409 "$(code -X POST $B/auth/register -H "$J" -d '{"username":"ADMIN1","password":"Str0ng!Passphrase42"}')"
check "wrong password"               401 "$(code -X POST $B/auth/login -H "$J" -d '{"username":"admin1","password":"nope"}')"
T=$(curl -s -X POST $B/auth/login -H "$J" -d '{"username":"admin1","password":"Str0ng!Passphrase42"}' | jget 'd["token"]')
U=$(curl -s -X POST $B/auth/login -H "$J" -d '{"username":"user1","password":"Another#Str0ng99pw"}' | jget 'd["token"]')
A="Authorization: Bearer $T"; UA="Authorization: Bearer $U"
check "me"                           200 "$(code -H "$A" $B/auth/me)"
check "password analyze"             200 "$(code -X POST $B/password/analyze -H "$A" -H "$J" -d '{"password":"Tr0ub4dor&3xyz"}')"
check "password analyze empty"       400 "$(code -X POST $B/password/analyze -H "$A" -H "$J" -d '{"password":""}')"
check "url analyze"                  200 "$(code -X POST $B/url/analyze -H "$A" -H "$J" -d '{"url":"http://192.168.1.5/login"}')"
check "url analyze bad scheme"       400 "$(code -X POST $B/url/analyze -H "$A" -H "$J" -d '{"url":"ftp://x.com"}')"
check "scan requires authorized"     400 "$(code -X POST $B/network/scan -H "$A" -H "$J" -d '{"host":"127.0.0.1","startPort":1,"endPort":10}')"
check "scan loopback"                200 "$(code -X POST $B/network/scan -H "$A" -H "$J" -d '{"host":"127.0.0.1","startPort":1,"endPort":100,"authorized":true}')"
check "scan public host blocked"     403 "$(code -X POST $B/network/scan -H "$A" -H "$J" -d '{"host":"8.8.8.8","startPort":53,"endPort":53,"authorized":true}')"
check "scan >1024 ports rejected"    400 "$(code -X POST $B/network/scan -H "$A" -H "$J" -d '{"host":"127.0.0.1","startPort":1,"endPort":2000,"authorized":true}')"
echo hello > "$TMP/n.txt"; printf 'MZ\x90\x00%0100d' 0 > "$TMP/invoice.pdf.exe"
check "file hash"                    200 "$(code -X POST $B/file/hash -H "$A" -F file=@$TMP/n.txt)"
check "file hash w/o file"           400 "$(code -X POST $B/file/hash -H "$A")"
V=$(curl -s -X POST $B/file/analyze -H "$A" -F file=@$TMP/invoice.pdf.exe | jget 'd["verdict"]')
check "disguised exe is High Risk"   "High Risk" "$V"
S=$(curl -s -X POST $B/file/hash -H "$A" -F file=@$TMP/n.txt | jget 'd["hashes"]["sha256"]')
check "sha256 matches sha256sum"     "$(sha256sum $TMP/n.txt | cut -d' ' -f1)" "$S"
check "integrity baseline"           201 "$(code -X POST $B/file/integrity/baseline -H "$A" -F file=@$TMP/n.txt)"
check "integrity intact"             intact "$(curl -s -X POST $B/file/integrity/verify -H "$A" -F file=@$TMP/n.txt | jget 'd["status"]')"
echo tamper >> "$TMP/n.txt"
check "integrity detects change"     modified "$(curl -s -X POST $B/file/integrity/verify -H "$A" -F file=@$TMP/n.txt | jget 'd["status"]')"
check "other user cannot delete"     404 "$(code -X DELETE -H "$UA" $B/file/integrity/1)"
check "owner deletes baseline"       200 "$(code -X DELETE -H "$A" $B/file/integrity/1)"
check "dashboard"                    200 "$(code -H "$A" $B/dashboard)"
check "dashboard scope=all as user"  403 "$(code -H "$UA" "$B/dashboard?scope=all")"
check "logs"                         200 "$(code -H "$A" "$B/logs?severity=critical&pageSize=5")"
check "report generate"              201 "$(code -X POST $B/reports/generate -H "$A" -H "$J" -d '{"title":"Test"}')"
check "reports list"                 200 "$(code -H "$A" $B/reports)"
check "admin users as admin"         200 "$(code -H "$A" $B/admin/users)"
check "admin users as user"          403 "$(code -H "$UA" $B/admin/users)"
check "unknown endpoint"             404 "$(code $B/nope)"
check "logout"                       200 "$(code -X POST -H "$UA" $B/auth/logout)"
check "token invalid after logout"   401 "$(code -H "$UA" $B/auth/me)"
echo; echo "passed=$pass failed=$fail"; [ "$fail" = 0 ]
