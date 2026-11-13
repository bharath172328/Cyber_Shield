#include "security/FileIntegrityMonitor.h"
#include "utils/Crypto.h"
#include "utils/StringUtil.h"

using nlohmann::json;

json FileIntegrityMonitor::setBaseline(int64_t userId, const std::string& filename, const std::string& sha256) {
    db_.execute(
        "INSERT INTO file_integrity(user_id,filename,baseline_hash,status) VALUES(?,?,?, 'baseline') "
        "ON CONFLICT(user_id, filename) DO UPDATE SET baseline_hash = excluded.baseline_hash, current_hash = NULL, "
        "status = 'baseline', last_checked = NULL, created_at = strftime('%Y-%m-%dT%H:%M:%SZ','now')",
        {(long long)userId, filename, sha256});
    return {{"filename", filename}, {"baselineHash", sha256}, {"status", "baseline"}};
}

json FileIntegrityMonitor::verify(int64_t userId, const std::string& filename, const std::string& sha256,
                                  const std::optional<std::string>& expectedHash) {
    if (expectedHash) {
        bool match = crypto::constantTimeEquals(util::toLower(util::trim(*expectedHash)), sha256);
        return {{"filename", filename}, {"status", match ? "intact" : "modified"}, {"match", match},
                {"baselineHash", util::toLower(util::trim(*expectedHash))}, {"currentHash", sha256}, {"source", "provided"}};
    }
    auto rows = db_.query("SELECT id, baseline_hash FROM file_integrity WHERE user_id = ? AND filename = ?",
                          {(long long)userId, filename});
    if (rows.empty())
        return {{"filename", filename}, {"status", "no_baseline"}, {"match", nullptr}, {"currentHash", sha256}, {"source", "stored"}};
    std::string baseline = rows[0]["baseline_hash"].get<std::string>();
    bool match = crypto::constantTimeEquals(baseline, sha256);
    db_.execute("UPDATE file_integrity SET current_hash = ?, status = ?, last_checked = strftime('%Y-%m-%dT%H:%M:%SZ','now') WHERE id = ?",
                {sha256, match ? "intact" : "modified", rows[0]["id"].get<int64_t>()});
    return {{"filename", filename}, {"status", match ? "intact" : "modified"}, {"match", match},
            {"baselineHash", baseline}, {"currentHash", sha256}, {"source", "stored"}};
}

json FileIntegrityMonitor::list(int64_t userId, bool all) {
    if (all) return db_.query("SELECT * FROM file_integrity ORDER BY created_at DESC LIMIT 500");
    return db_.query("SELECT * FROM file_integrity WHERE user_id = ? ORDER BY created_at DESC LIMIT 500", {(long long)userId});
}

bool FileIntegrityMonitor::remove(int64_t userId, int64_t id, bool isAdmin) {
    auto rows = isAdmin ? db_.query("SELECT id FROM file_integrity WHERE id = ?", {(long long)id})
                        : db_.query("SELECT id FROM file_integrity WHERE id = ? AND user_id = ?", {(long long)id, (long long)userId});
    if (rows.empty()) return false;
    db_.execute("DELETE FROM file_integrity WHERE id = ?", {(long long)id});
    return true;
}
