#include "authentication/AuthenticationManager.h"
#include "security/PasswordAnalyzer.h"
#include "utils/Crypto.h"
#include "utils/StringUtil.h"

using namespace std::chrono;

static const size_t kMaxFailures = 5;
static const auto kLockWindow = minutes(10);

AuthenticationManager::AuthenticationManager(DatabaseManager& db, int ttl)
    : db_(db), ttlHours_(ttl), dummyHash_(crypto::hashPassword("cyber-shield-dummy")) {}

static bool validUsername(const std::string& u) {
    if (u.size() < 3 || u.size() > 32) return false;
    for (unsigned char c : u)
        if (!(std::isalnum(c) || c == '_' || c == '.' || c == '-')) return false;
    return true;
}

static bool validEmail(const std::string& e) {
    if (e.size() > 254) return false;
    auto at = e.find('@');
    return at != std::string::npos && at > 0 && e.find('.', at) != std::string::npos && e.find(' ') == std::string::npos;
}

AuthUser AuthenticationManager::registerUser(const std::string& username, const std::string& email,
                                             const std::string& password) {
    if (!validUsername(username)) throw AuthError(400, "Username must be 3-32 characters: letters, numbers, . _ -");
    if (!email.empty() && !validEmail(email)) throw AuthError(400, "Email address is not valid");
    if (password.size() < 8 || password.size() > 128) throw AuthError(400, "Password must be 8-128 characters");
    if (PasswordAnalyzer().analyze(password).score < 50)
        throw AuthError(400, "Password is too weak. Use a longer mix of letters, numbers and symbols");

    auto existing = db_.query("SELECT id FROM users WHERE username = ?", {username});
    if (!existing.empty()) throw AuthError(409, "Username is already taken");

    // The very first account becomes the administrator.
    auto count = db_.query("SELECT COUNT(*) AS c FROM users");
    std::string role = count[0]["c"].get<int64_t>() == 0 ? "admin" : "user";
    int64_t id = db_.insert("INSERT INTO users(username,email,password_hash,role) VALUES(?,?,?,?)",
                            {username, email.empty() ? Param(nullptr) : Param(email), crypto::hashPassword(password), role});
    return {id, username, role};
}

bool AuthenticationManager::isLockedOut(const std::string& key) {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = failures_.find(key);
    if (it == failures_.end()) return false;
    auto now = steady_clock::now();
    while (!it->second.empty() && now - it->second.front() > kLockWindow) it->second.pop_front();
    return it->second.size() >= kMaxFailures;
}

void AuthenticationManager::recordFailure(const std::string& key) {
    std::lock_guard<std::mutex> lk(mu_);
    failures_[key].push_back(steady_clock::now());
}

AuthResult AuthenticationManager::login(const std::string& username, const std::string& password,
                                        const std::string& ip) {
    std::string key = util::toLower(username) + "|" + ip;
    if (isLockedOut(key)) throw AuthError(429, "Too many failed attempts. Try again in 10 minutes");

    auto rows = db_.query("SELECT id, username, role, password_hash FROM users WHERE username = ?", {username});
    bool ok = false;
    if (rows.empty()) {
        crypto::verifyPassword(password, dummyHash_);  // constant-ish timing
    } else {
        ok = crypto::verifyPassword(password, rows[0]["password_hash"].get<std::string>());
    }
    if (!ok) {
        recordFailure(key);
        throw AuthError(401, "Invalid username or password");
    }
    { std::lock_guard<std::mutex> lk(mu_); failures_.erase(key); }

    AuthUser user{rows[0]["id"].get<int64_t>(), rows[0]["username"].get<std::string>(),
                  rows[0]["role"].get<std::string>()};
    std::string token = crypto::randomHex(32);
    std::string modifier = "+" + std::to_string(ttlHours_) + " hours";
    db_.insert("INSERT INTO sessions(token_hash,user_id,expires_at) VALUES(?,?,strftime('%Y-%m-%dT%H:%M:%SZ','now',?))",
               {crypto::digestHex(token, "SHA256"), (long long)user.id, modifier});
    db_.execute("UPDATE users SET last_login = strftime('%Y-%m-%dT%H:%M:%SZ','now') WHERE id = ?", {(long long)user.id});
    auto exp = db_.query("SELECT expires_at FROM sessions WHERE token_hash = ?", {crypto::digestHex(token, "SHA256")});
    return {token, user, exp.empty() ? "" : exp[0]["expires_at"].get<std::string>()};
}

std::optional<AuthUser> AuthenticationManager::authenticate(const std::string& token) {
    if (token.size() != 64) return std::nullopt;
    auto rows = db_.query(
        "SELECT u.id, u.username, u.role FROM sessions s JOIN users u ON u.id = s.user_id "
        "WHERE s.token_hash = ? AND s.expires_at > strftime('%Y-%m-%dT%H:%M:%SZ','now')",
        {crypto::digestHex(token, "SHA256")});
    if (rows.empty()) return std::nullopt;
    return AuthUser{rows[0]["id"].get<int64_t>(), rows[0]["username"].get<std::string>(),
                    rows[0]["role"].get<std::string>()};
}

void AuthenticationManager::logout(const std::string& token) {
    db_.execute("DELETE FROM sessions WHERE token_hash = ?", {crypto::digestHex(token, "SHA256")});
}

void AuthenticationManager::changePassword(int64_t userId, const std::string& current, const std::string& next) {
    auto rows = db_.query("SELECT password_hash FROM users WHERE id = ?", {(long long)userId});
    if (rows.empty() || !crypto::verifyPassword(current, rows[0]["password_hash"].get<std::string>()))
        throw AuthError(401, "Current password is incorrect");
    if (next.size() < 8 || next.size() > 128 || PasswordAnalyzer().analyze(next).score < 50)
        throw AuthError(400, "New password is too weak");
    db_.execute("UPDATE users SET password_hash = ? WHERE id = ?", {crypto::hashPassword(next), (long long)userId});
}

void AuthenticationManager::purgeExpired() {
    db_.execute("DELETE FROM sessions WHERE expires_at <= strftime('%Y-%m-%dT%H:%M:%SZ','now')");
}
