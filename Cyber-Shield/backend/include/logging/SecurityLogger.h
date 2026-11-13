#pragma once
#include <cstdint>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>
#include "database/DatabaseManager.h"

// Writes every security-relevant event to the database AND to a rotating-friendly log file.
// Severity: info | warning | error | critical
class SecurityLogger {
public:
    SecurityLogger(DatabaseManager& db, const std::string& logFilePath);
    void log(const std::string& eventType, const std::string& severity, const std::string& description,
             std::optional<int64_t> userId = std::nullopt, const std::string& ip = "");

private:
    DatabaseManager& db_;
    std::ofstream file_;
    std::mutex mu_;
};
