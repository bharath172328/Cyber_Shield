#include "logging/SecurityLogger.h"
#include <ctime>
#include <filesystem>
#include <iostream>

static std::string utcNow() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buf;
}

// Strip control characters so attacker-supplied text cannot forge extra log lines.
static std::string sanitize(const std::string& s) {
    std::string out;
    for (unsigned char c : s) out.push_back(c < 0x20 || c == 0x7f ? ' ' : (char)c);
    if (out.size() > 500) out.resize(500);
    return out;
}

SecurityLogger::SecurityLogger(DatabaseManager& db, const std::string& path) : db_(db) {
    try {
        auto parent = std::filesystem::path(path).parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);
        file_.open(path, std::ios::app);
    } catch (const std::exception& e) {
        std::cerr << "[logger] cannot open log file: " << e.what() << std::endl;
    }
}

void SecurityLogger::log(const std::string& type, const std::string& severity, const std::string& desc,
                         std::optional<int64_t> userId, const std::string& ip) {
    std::string d = sanitize(desc);
    {
        std::lock_guard<std::mutex> lk(mu_);
        std::string line = utcNow() + " [" + severity + "] " + type + " user=" +
                           (userId ? std::to_string(*userId) : "-") + " ip=" + (ip.empty() ? "-" : ip) + " " + d;
        std::cout << line << std::endl;
        if (file_.is_open()) file_ << line << std::endl;
    }
    try {
        Param uid = userId ? Param((long long)*userId) : Param(nullptr);
        db_.insert("INSERT INTO security_logs(user_id,event_type,severity,description,ip_address) VALUES(?,?,?,?,?)",
                   {uid, type, severity, d, ip});
    } catch (const std::exception& e) {
        std::cerr << "[logger] DB write failed: " << e.what() << std::endl;
    }
}
