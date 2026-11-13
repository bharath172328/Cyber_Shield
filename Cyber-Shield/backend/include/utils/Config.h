#pragma once
#include <string>
#include <vector>

// Runtime configuration, read from environment variables (no hardcoded secrets).
struct Config {
    int port = 8080;
    std::vector<std::string> corsOrigins{"http://localhost:5173", "http://127.0.0.1:5173"};
    std::string dbPath = "cybershield.db";
    std::string schemaPath;
    std::string frontendDir;  // built React app (frontend/dist), served at /
    std::string logPath = "logs/cybershield.log";
    std::vector<std::string> scanAllowedHosts;  // extra hosts the operator authorises
    int sessionTtlHours = 12;
    size_t maxUploadBytes = 25 * 1024 * 1024;

    static Config fromEnv();
};
