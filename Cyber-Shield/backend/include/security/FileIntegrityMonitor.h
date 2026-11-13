#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include "database/DatabaseManager.h"

// Stores a trusted baseline hash per file and later checks whether the file still matches it.
class FileIntegrityMonitor {
public:
    explicit FileIntegrityMonitor(DatabaseManager& db) : db_(db) {}
    nlohmann::json setBaseline(int64_t userId, const std::string& filename, const std::string& sha256);
    // If expectedHash is given, compare against it; otherwise use the stored baseline.
    nlohmann::json verify(int64_t userId, const std::string& filename, const std::string& sha256,
                          const std::optional<std::string>& expectedHash);
    nlohmann::json list(int64_t userId, bool all);
    bool remove(int64_t userId, int64_t id, bool isAdmin);

private:
    DatabaseManager& db_;
};
