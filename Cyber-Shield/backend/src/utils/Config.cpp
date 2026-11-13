#include "utils/Config.h"
#include <cstdlib>
#include <filesystem>
#include "utils/StringUtil.h"

namespace fs = std::filesystem;

#ifndef CS_FRONTEND_DIR
#define CS_FRONTEND_DIR "../frontend/dist"
#endif

#ifndef CS_SCHEMA_PATH
#define CS_SCHEMA_PATH "database/schema.sql"
#endif

static std::string env(const char* k) {
    const char* v = std::getenv(k);
    return v ? std::string(v) : std::string();
}

static std::vector<std::string> csv(const std::string& s) {
    std::vector<std::string> out;
    for (auto& p : util::split(s, ',')) {
        auto t = util::trim(p);
        if (!t.empty()) out.push_back(t);
    }
    return out;
}

Config Config::fromEnv() {
    Config c;
    c.schemaPath = CS_SCHEMA_PATH;
    c.frontendDir = CS_FRONTEND_DIR;
    if (auto v = env("CS_FRONTEND_DIR"); !v.empty()) c.frontendDir = v;
    if (auto v = env("CS_PORT"); !v.empty()) {
        int p = std::atoi(v.c_str());
        if (p > 0 && p < 65536) c.port = p;
    }
    if (auto v = env("CS_CORS_ORIGINS"); !v.empty()) c.corsOrigins = csv(v);
    if (auto v = env("CS_DB_PATH"); !v.empty()) c.dbPath = v;
    if (auto v = env("CS_SCHEMA_PATH"); !v.empty()) c.schemaPath = v;
    if (auto v = env("CS_LOG_PATH"); !v.empty()) c.logPath = v;
    if (auto v = env("CS_SCAN_ALLOWED_HOSTS"); !v.empty()) c.scanAllowedHosts = csv(v);
    if (auto v = env("CS_SESSION_TTL_HOURS"); !v.empty()) {
        int h = std::atoi(v.c_str());
        if (h > 0 && h <= 720) c.sessionTtlHours = h;
    }

    // Fallback if configured schemaPath does not exist
    if (!c.schemaPath.empty()) {
        std::error_code ec;
        if (!fs::exists(c.schemaPath, ec)) {
            const std::string candidates[] = {
                "database/schema.sql",
                "backend/database/schema.sql",
                "Cyber-Shield/backend/database/schema.sql",
                "../backend/database/schema.sql"
            };
            for (const auto& candidate : candidates) {
                if (fs::exists(candidate, ec)) {
                    c.schemaPath = candidate;
                    break;
                }
            }
        }
    }

    // Fallback if configured frontendDir does not exist
    if (!c.frontendDir.empty()) {
        std::error_code ec;
        if (!fs::exists(c.frontendDir, ec)) {
            const std::string candidates[] = {
                "../frontend/dist",
                "frontend/dist",
                "Cyber-Shield/frontend/dist"
            };
            for (const auto& candidate : candidates) {
                if (fs::exists(candidate, ec)) {
                    c.frontendDir = candidate;
                    break;
                }
            }
        }
    }

    return c;
}
