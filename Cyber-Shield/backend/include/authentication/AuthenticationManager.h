#pragma once
#include <chrono>
#include <cstdint>
#include <deque>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include "database/DatabaseManager.h"

struct AuthUser {
    int64_t id = 0;
    std::string username;
    std::string role;
};

struct AuthResult {
    std::string token;
    AuthUser user;
    std::string expiresAt;
};

// Carries an HTTP status so controllers can map it directly.
class AuthError : public std::runtime_error {
public:
    AuthError(int status, const std::string& msg) : std::runtime_error(msg), status_(status) {}
    int status() const { return status_; }
private:
    int status_;
};

class AuthenticationManager {
public:
    AuthenticationManager(DatabaseManager& db, int sessionTtlHours);

    AuthUser registerUser(const std::string& username, const std::string& email, const std::string& password);
    AuthResult login(const std::string& username, const std::string& password, const std::string& ip);
    std::optional<AuthUser> authenticate(const std::string& token);
    void logout(const std::string& token);
    void changePassword(int64_t userId, const std::string& current, const std::string& next);
    void purgeExpired();

private:
    bool isLockedOut(const std::string& key);
    void recordFailure(const std::string& key);

    DatabaseManager& db_;
    int ttlHours_;
    std::string dummyHash_;  // used to equalise timing for unknown usernames
    std::mutex mu_;
    std::map<std::string, std::deque<std::chrono::steady_clock::time_point>> failures_;
};
