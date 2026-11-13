#include "api/Controllers.h"
#include "api/HttpUtil.h"

using nlohmann::json;

namespace {
json findingsJson(const std::vector<FileFinding>& f) {
    json a = json::array();
    for (auto& x : f) a.push_back({{"id", x.id}, {"description", x.description}, {"severity", x.severity}, {"weight", x.weight}});
    return a;
}

// Reads the uploaded "file" part. Writes an error response and returns false on failure.
bool readUpload(const httplib::Request& req, httplib::Response& res, std::string& name, std::string& data) {
    if (!req.has_file("file")) { http::sendError(res, 400, "Multipart field 'file' is required"); return false; }
    const auto& f = req.get_file_value("file");
    name = FileHasher::sanitizeFilename(f.filename);
    data = f.content;
    return true;
}
}  // namespace

void registerSecurityRoutes(httplib::Server& svr, ApiContext& ctx) {
    // ---------------------------------------------------------------- password
    svr.Post("/api/password/analyze", [&ctx](const httplib::Request& req, httplib::Response& res) {
        auto user = http::requireAuth(ctx, req, res);
        if (!user) return;
        auto body = http::parseBody(req, res);
        if (!body) return;
        if (!body->value("password", json()).is_string()) return http::sendError(res, 400, "Field 'password' (string) is required");
        std::string pw = (*body)["password"];
        if (pw.empty() || pw.size() > 256) return http::sendError(res, 400, "Password must be 1-256 characters");

        auto r = ctx.passwords.analyze(pw);
        json checks = json::array();
        for (auto& c : r.checks) checks.push_back({{"name", c.name}, {"passed", c.passed}, {"detail", c.detail}});
        // The password itself is never stored or logged - only the score.
        ctx.db.insert("INSERT INTO password_analysis(user_id,score,strength,length) VALUES(?,?,?,?)",
                      {(long long)user->id, r.score, r.strength, (long long)pw.size()});
        ctx.logger.log("PASSWORD_ANALYSIS", r.score < 50 ? "warning" : "info",
                       "Password analysed: score " + std::to_string(r.score) + " (" + r.strength + ")", user->id, http::clientIp(req));
        http::sendJson(res, 200, {{"score", r.score}, {"strength", r.strength}, {"message", r.message},
                                  {"entropyBits", r.entropyBits}, {"findings", r.findings}, {"checks", checks}});
    });

    // --------------------------------------------------------------------- url
    svr.Post("/api/url/analyze", [&ctx](const httplib::Request& req, httplib::Response& res) {
        auto user = http::requireAuth(ctx, req, res);
        if (!user) return;
        auto body = http::parseBody(req, res);
        if (!body) return;
        if (!body->value("url", json()).is_string()) return http::sendError(res, 400, "Field 'url' (string) is required");
        try {
            auto r = ctx.urls.analyze((*body)["url"]);
            json ind = json::array();
            for (auto& i : r.indicators) ind.push_back({{"id", i.id}, {"description", i.description}, {"severity", i.severity}, {"weight", i.weight}});
            ctx.db.insert("INSERT INTO url_analysis(user_id,url,risk_score,risk_level,indicators_json) VALUES(?,?,?,?,?)",
                          {(long long)user->id, r.url, r.riskScore, r.riskLevel, ind.dump()});
            bool bad = r.riskLevel == "High" || r.riskLevel == "Critical";
            ctx.logger.log("URL_ANALYSIS", r.riskLevel == "Critical" ? "critical" : (bad || r.riskLevel == "Medium" ? "warning" : "info"),
                           "URL analysed (" + r.host + "): " + r.riskLevel + " risk, score " + std::to_string(r.riskScore), user->id, http::clientIp(req));
            http::sendJson(res, 200, {{"url", r.url}, {"host", r.host}, {"scheme", r.scheme}, {"port", r.port}, {"path", r.path},
                                      {"riskScore", r.riskScore}, {"riskLevel", r.riskLevel}, {"indicators", ind},
                                      {"recommendation", r.recommendation}});
        } catch (const std::invalid_argument& e) {
            http::sendError(res, 400, e.what());
        }
    });

    // ----------------------------------------------------------------- network
    svr.Post("/api/network/scan", [&ctx](const httplib::Request& req, httplib::Response& res) {
        auto user = http::requireAuth(ctx, req, res);
        if (!user) return;
        auto body = http::parseBody(req, res);
        if (!body) return;
        if (!body->value("host", json()).is_string() || !body->value("startPort", json()).is_number_integer() ||
            !body->value("endPort", json()).is_number_integer())
            return http::sendError(res, 400, "Fields 'host' (string), 'startPort' and 'endPort' (integers) are required");
        if (body->value("authorized", false) != true)
            return http::sendError(res, 400, "You must confirm you are authorised to scan this host ('authorized': true)");
        std::string host = (*body)["host"];
        int startPort = (*body)["startPort"], endPort = (*body)["endPort"];
        try {
            auto r = ctx.scanner.scan(host, startPort, endPort);
            json ports = json::array(), open = json::array();
            for (auto& p : r.ports) {
                json pj = {{"port", p.port}, {"state", p.state}, {"service", p.service}, {"risk", p.risk}, {"note", p.note}};
                ports.push_back(pj);
                if (p.state == "open") open.push_back(pj);
            }
            ctx.db.insert("INSERT INTO network_scans(user_id,host,port_start,port_end,open_count,risky_open_count,open_ports_json,duration_ms) VALUES(?,?,?,?,?,?,?,?)",
                          {(long long)user->id, r.host, r.startPort, r.endPort, r.openCount, r.riskyOpenCount, open.dump(), (long long)r.durationMs});
            ctx.logger.log("NETWORK_SCAN", r.riskyOpenCount > 0 ? "warning" : "info",
                           "Scan of " + r.host + " ports " + std::to_string(startPort) + "-" + std::to_string(endPort) + ": " +
                               std::to_string(r.openCount) + " open, " + std::to_string(r.riskyOpenCount) + " risky", user->id, http::clientIp(req));
            http::sendJson(res, 200, {{"host", r.host}, {"resolvedIp", r.resolvedIp}, {"startPort", r.startPort}, {"endPort", r.endPort},
                                      {"durationMs", r.durationMs}, {"openCount", r.openCount}, {"closedCount", r.closedCount},
                                      {"filteredCount", r.filteredCount}, {"riskyOpenCount", r.riskyOpenCount}, {"ports", ports}});
        } catch (const ScanPolicyError& e) {
            ctx.logger.log("SECURITY_EVENT", "warning", "Blocked scan of non-permitted host: " + host, user->id, http::clientIp(req));
            http::sendError(res, 403, e.what());
        } catch (const ScanBusyError& e) {
            http::sendError(res, 429, e.what());
        } catch (const std::invalid_argument& e) {
            http::sendError(res, 400, e.what());
        }
    });

    // -------------------------------------------------------------------- file
    svr.Post("/api/file/hash", [&ctx](const httplib::Request& req, httplib::Response& res) {
        auto user = http::requireAuth(ctx, req, res);
        if (!user) return;
        std::string name, data;
        if (!readUpload(req, res, name, data)) return;
        auto h = ctx.hasher.hash(name, data);
        auto a = ctx.fileAnalyzer.analyze(name, data);
        ctx.logger.log("FILE_HASH", "info", "Hashed file " + h.filename + " (" + std::to_string(h.size) + " bytes)", user->id, http::clientIp(req));
        http::sendJson(res, 200, {{"filename", h.filename}, {"size", h.size},
                                  {"hashes", {{"sha256", h.sha256}, {"sha1", h.sha1}, {"md5", h.md5}}},
                                  {"md5Note", "MD5 and SHA-1 are broken for security purposes; use them only to identify files. Rely on SHA-256 for verification."},
                                  {"fileType", a.fileType}, {"analysisResult", a.verdict}, {"riskScore", a.riskScore}});
    });

    svr.Post("/api/file/analyze", [&ctx](const httplib::Request& req, httplib::Response& res) {
        auto user = http::requireAuth(ctx, req, res);
        if (!user) return;
        std::string name, data;
        if (!readUpload(req, res, name, data)) return;
        auto h = ctx.hasher.hash(name, data);
        auto a = ctx.fileAnalyzer.analyze(name, data);
        json findings = findingsJson(a.findings);
        ctx.db.insert("INSERT INTO file_analysis(user_id,filename,size,sha256,file_type,risk_score,verdict,findings_json) VALUES(?,?,?,?,?,?,?,?)",
                      {(long long)user->id, h.filename, (long long)h.size, h.sha256, a.fileType, a.riskScore, a.verdict, findings.dump()});
        ctx.logger.log("FILE_ANALYSIS", a.verdict == "High Risk" ? "critical" : (a.verdict == "Suspicious" ? "warning" : "info"),
                       "File analysed: " + h.filename + " -> " + a.verdict + " (score " + std::to_string(a.riskScore) + ")", user->id, http::clientIp(req));
        http::sendJson(res, 200, {{"filename", h.filename}, {"size", h.size}, {"sha256", h.sha256}, {"fileType", a.fileType},
                                  {"extension", a.extension}, {"entropy", a.entropy}, {"riskScore", a.riskScore},
                                  {"verdict", a.verdict}, {"findings", findings}});
    });

    // --------------------------------------------------------------- integrity
    svr.Get("/api/file/integrity", [&ctx](const httplib::Request& req, httplib::Response& res) {
        auto user = http::requireAuth(ctx, req, res);
        if (!user) return;
        http::sendJson(res, 200, {{"items", ctx.integrity.list(user->id, false)}});
    });

    svr.Post("/api/file/integrity/baseline", [&ctx](const httplib::Request& req, httplib::Response& res) {
        auto user = http::requireAuth(ctx, req, res);
        if (!user) return;
        std::string name, data;
        if (!readUpload(req, res, name, data)) return;
        auto h = ctx.hasher.hash(name, data);
        auto r = ctx.integrity.setBaseline(user->id, h.filename, h.sha256);
        ctx.logger.log("FILE_INTEGRITY", "info", "Baseline stored for " + h.filename, user->id, http::clientIp(req));
        http::sendJson(res, 201, r);
    });

    svr.Post("/api/file/integrity/verify", [&ctx](const httplib::Request& req, httplib::Response& res) {
        auto user = http::requireAuth(ctx, req, res);
        if (!user) return;
        std::string name, data;
        if (!readUpload(req, res, name, data)) return;
        std::optional<std::string> expected;
        if (req.has_file("expectedHash")) {
            std::string e = req.get_file_value("expectedHash").content;
            if (!e.empty()) expected = e;
        }
        auto h = ctx.hasher.hash(name, data);
        auto r = ctx.integrity.verify(user->id, h.filename, h.sha256, expected);
        std::string status = r["status"];
        ctx.logger.log("FILE_INTEGRITY", status == "modified" ? "critical" : (status == "intact" ? "info" : "warning"),
                       "Integrity check of " + h.filename + ": " + status, user->id, http::clientIp(req));
        http::sendJson(res, 200, r);
    });

    svr.Delete(R"(/api/file/integrity/(\d+))", [&ctx](const httplib::Request& req, httplib::Response& res) {
        auto user = http::requireAuth(ctx, req, res);
        if (!user) return;
        int64_t id = std::stoll(req.matches[1]);
        if (!ctx.integrity.remove(user->id, id, user->role == "admin")) return http::sendError(res, 404, "Baseline not found");
        ctx.logger.log("FILE_INTEGRITY", "info", "Baseline #" + std::to_string(id) + " deleted", user->id, http::clientIp(req));
        http::sendJson(res, 200, {{"message", "Baseline deleted"}});
    });
}
