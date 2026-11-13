#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include "database/DatabaseManager.h"

// Aggregates results of all analyzers into a security score, statistics and recommendations.
class ThreatDetector {
public:
    explicit ThreatDetector(DatabaseManager& db) : db_(db) {}
    nlohmann::json dashboard(int64_t userId, bool allUsers) const;
    nlohmann::json recommendations(const nlohmann::json& dashboard) const;
    static std::string scoreLabel(int score);

private:
    DatabaseManager& db_;
};
