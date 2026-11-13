#include "security/ThreatDetector.h"
#include <algorithm>
#include <map>
#include <string>

using nlohmann::json;

std::string ThreatDetector::scoreLabel(int s) {
    if (s >= 85) return "Secure";
    if (s >= 65) return "Good";
    if (s >= 40) return "At Risk";
    return "Critical";
}

json ThreatDetector::dashboard(int64_t userId, bool all) const {
    const std::string uf = all ? "1=1" : "user_id = ?";
    auto P = [&](std::vector<Param> extra = {}) {
        std::vector<Param> p;
        if (!all) p.push_back((long long)userId);
        for (auto& e : extra) p.push_back(e);
        return p;
    };
    auto count = [&](const std::string& table, const std::string& extraWhere = "") {
        auto r = db_.query("SELECT COUNT(*) AS c FROM " + table + " WHERE " + uf + extraWhere, P());
        return r[0]["c"].get<int64_t>();
    };
    auto grouped = [&](const std::string& table, const std::string& col) {
        json out = json::object();
        for (auto& row : db_.query("SELECT " + col + " AS k, COUNT(*) AS c FROM " + table + " WHERE " + uf + " GROUP BY " + col, P()))
            out[row["k"].get<std::string>()] = row["c"];
        return out;
    };

    json d;
    // ---- password stats
    auto pw = db_.query("SELECT COUNT(*) AS total, COALESCE(AVG(score),0) AS avg FROM password_analysis WHERE " + uf, P())[0];
    int64_t weakPw = count("password_analysis", " AND score < 50");
    d["passwordStats"] = {{"total", pw["total"]}, {"averageScore", (int)(pw["avg"].get<double>() + 0.5)},
                          {"weak", weakPw}, {"byStrength", grouped("password_analysis", "strength")}};
    // ---- url stats
    int64_t urlHigh = count("url_analysis", " AND risk_level IN ('High','Critical')");
    d["urlStats"] = {{"total", count("url_analysis")}, {"highRisk", urlHigh}, {"byLevel", grouped("url_analysis", "risk_level")}};
    // ---- network stats
    auto net = db_.query("SELECT COUNT(*) AS total, COALESCE(SUM(open_count),0) AS open, COALESCE(SUM(risky_open_count),0) AS risky, "
                         "COALESCE(AVG(duration_ms),0) AS avgMs FROM network_scans WHERE " + uf, P())[0];
    d["networkStats"] = {{"total", net["total"]}, {"openPorts", net["open"]}, {"riskyOpenPorts", net["risky"]},
                         {"averageDurationMs", (int64_t)(net["avgMs"].get<double>() + 0.5)}};
    // ---- file stats
    int64_t fileThreats = count("file_analysis", " AND verdict != 'Clean'");
    d["fileStats"] = {{"total", count("file_analysis")}, {"threats", fileThreats}, {"byVerdict", grouped("file_analysis", "verdict")}};

    // ---- alerts
    const std::string since30 = " AND created_at >= strftime('%Y-%m-%dT%H:%M:%SZ','now','-30 days')";
    int64_t critical = count("security_logs", " AND severity = 'critical'" + since30);
    int64_t warnings = count("security_logs", " AND severity = 'warning'" + since30);
    int64_t riskyPorts = net["risky"].get<int64_t>();
    int64_t threats = urlHigh + fileThreats + (riskyPorts > 0 ? riskyPorts : 0);
    d["threatCount"] = threats;
    d["criticalAlerts"] = critical;
    d["warningAlerts"] = warnings;

    // ---- security score
    int score = 100;
    score -= (int)std::min<int64_t>(60, critical * 8);
    score -= (int)std::min<int64_t>(25, urlHigh * 4);
    score -= (int)std::min<int64_t>(25, fileThreats * 5);
    score -= (int)std::min<int64_t>(20, riskyPorts * 3);
    score -= (int)std::min<int64_t>(10, weakPw * 2);
    score = std::clamp(score, 0, 100);
    d["securityScore"] = score;
    d["securityLabel"] = scoreLabel(score);

    // ---- 7-day trend
    json days = db_.query("WITH RECURSIVE d(n) AS (SELECT 0 UNION ALL SELECT n+1 FROM d WHERE n < 6) "
                          "SELECT date('now', '-' || (6 - n) || ' days') AS day FROM d");
    std::map<std::string, json> trend;
    for (auto& row : days) {
        std::string day = row["day"];
        trend[day] = {{"date", day}, {"passwords", 0}, {"urls", 0}, {"scans", 0}, {"files", 0}, {"threats", 0}};
    }
    auto fill = [&](const std::string& table, const std::string& key, const std::string& extraWhere, const std::string& threatKey) {
        for (auto& row : db_.query("SELECT date(created_at) AS d, COUNT(*) AS c FROM " + table + " WHERE " + uf + extraWhere +
                                   " AND created_at >= date('now','-6 days') GROUP BY d", P())) {
            auto it = trend.find(row["d"].get<std::string>());
            if (it == trend.end()) continue;
            if (!key.empty()) it->second[key] = it->second[key].get<int64_t>() + row["c"].get<int64_t>();
            if (!threatKey.empty()) it->second["threats"] = it->second["threats"].get<int64_t>() + row["c"].get<int64_t>();
        }
    };
    fill("password_analysis", "passwords", "", "");
    fill("url_analysis", "urls", "", "");
    fill("network_scans", "scans", "", "");
    fill("file_analysis", "files", "", "");
    fill("url_analysis", "", " AND risk_level IN ('High','Critical')", "t");
    fill("file_analysis", "", " AND verdict != 'Clean'", "t");
    fill("network_scans", "", " AND risky_open_count > 0", "t");
    d["trend"] = json::array();
    for (auto& days_entry : days) d["trend"].push_back(trend[days_entry["day"].get<std::string>()]);

    // ---- recent events
    d["recentEvents"] = db_.query(
        "SELECT l.id, l.event_type, l.severity, l.description, l.created_at, u.username FROM security_logs l "
        "LEFT JOIN users u ON u.id = l.user_id WHERE " + std::string(all ? "1=1" : "l.user_id = ?") +
        " ORDER BY l.id DESC LIMIT 10", P());
    return d;
}

json ThreatDetector::recommendations(const json& d) const {
    json r = json::array();
    if (d["networkStats"]["riskyOpenPorts"].get<int64_t>() > 0)
        r.push_back("Close or firewall the high/medium-risk open ports found in network scans.");
    if (d["urlStats"]["highRisk"].get<int64_t>() > 0)
        r.push_back("Block the high-risk URLs identified and warn users about the associated phishing campaigns.");
    if (d["fileStats"]["threats"].get<int64_t>() > 0)
        r.push_back("Quarantine files flagged as Suspicious or High Risk and verify their origin.");
    if (d["passwordStats"]["weak"].get<int64_t>() > 0)
        r.push_back("Enforce longer passphrases (12+ characters) and enable multi-factor authentication.");
    if (d["criticalAlerts"].get<int64_t>() > 0)
        r.push_back("Investigate the critical security events recorded in the last 30 days.");
    if (r.empty()) r.push_back("No outstanding issues. Keep running regular scans and integrity checks.");
    return r;
}
