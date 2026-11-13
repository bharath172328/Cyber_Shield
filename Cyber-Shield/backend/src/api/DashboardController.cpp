#include "api/Controllers.h"
#include "api/HttpUtil.h"

using nlohmann::json;

// Only admins may look at every user's data (?scope=all).
static bool wantsAll(ApiContext& ctx, const httplib::Request& req, httplib::Response& res, const AuthUser& u, bool& all) {
    all = req.get_param_value("scope") == "all";
    if (all && u.role != "admin") {
        ctx.logger.log("SECURITY_EVENT", "warning", "Non-admin requested scope=all on " + req.path, u.id, http::clientIp(req));
        http::sendError(res, 403, "Administrator access required for scope=all");
        return false;
    }
    return true;
}

static json parseStored(const json& row, const char* col) {
    try { return json::parse(row[col].get<std::string>()); } catch (...) { return json::object(); }
}

void registerDashboardRoutes(httplib::Server& svr, ApiContext& ctx) {
    svr.Get("/api/dashboard", [&ctx](const httplib::Request& req, httplib::Response& res) {
        auto user = http::requireAuth(ctx, req, res);
        if (!user) return;
        bool all;
        if (!wantsAll(ctx, req, res, *user, all)) return;
        http::sendJson(res, 200, ctx.threats.dashboard(user->id, all));
    });

    svr.Get("/api/logs", [&ctx](const httplib::Request& req, httplib::Response& res) {
        auto user = http::requireAuth(ctx, req, res);
        if (!user) return;
        bool all;
        if (!wantsAll(ctx, req, res, *user, all)) return;
        int page = http::queryInt(req, "page", 1, 1, 100000);
        int pageSize = http::queryInt(req, "pageSize", 20, 1, 100);

        std::string where = all ? "1=1" : "l.user_id = ?";
        std::vector<Param> params;
        if (!all) params.push_back((long long)user->id);
        std::string sev = req.get_param_value("severity"), type = req.get_param_value("type"), q = req.get_param_value("q");
        if (sev == "info" || sev == "warning" || sev == "error" || sev == "critical") { where += " AND l.severity = ?"; params.push_back(sev); }
        if (!type.empty() && type.size() <= 40) { where += " AND l.event_type = ?"; params.push_back(type); }
        if (!q.empty() && q.size() <= 100) {
            std::string esc;
            for (char c : q) { if (c == '%' || c == '_' || c == '\\') esc.push_back('\\'); esc.push_back(c); }
            where += " AND l.description LIKE ? ESCAPE '\\'";
            params.push_back("%" + esc + "%");
        }
        auto total = ctx.db.query("SELECT COUNT(*) AS c FROM security_logs l WHERE " + where, params)[0]["c"].get<int64_t>();
        auto p2 = params;
        p2.push_back(pageSize);
        p2.push_back((page - 1) * pageSize);
        auto items = ctx.db.query(
            "SELECT l.id, l.event_type, l.severity, l.description, l.ip_address, l.created_at, u.username FROM security_logs l "
            "LEFT JOIN users u ON u.id = l.user_id WHERE " + where + " ORDER BY l.id DESC LIMIT ? OFFSET ?", p2);
        http::sendJson(res, 200, {{"items", items}, {"total", total}, {"page", page}, {"pageSize", pageSize}});
    });

    svr.Get("/api/reports", [&ctx](const httplib::Request& req, httplib::Response& res) {
        auto user = http::requireAuth(ctx, req, res);
        if (!user) return;
        auto rows = ctx.db.query("SELECT id, title, summary_json, created_at FROM reports WHERE user_id = ? ORDER BY id DESC LIMIT 50", {(long long)user->id});
        json items = json::array();
        for (auto& r : rows) items.push_back({{"id", r["id"]}, {"title", r["title"]}, {"createdAt", r["created_at"]}, {"summary", parseStored(r, "summary_json")}});
        http::sendJson(res, 200, {{"items", items}});
    });

    svr.Post("/api/reports/generate", [&ctx](const httplib::Request& req, httplib::Response& res) {
        auto user = http::requireAuth(ctx, req, res);
        if (!user) return;
        std::string title = "Security Report";
        if (!req.body.empty()) {
            try {
                auto j = json::parse(req.body);
                if (j.is_object() && j.value("title", json()).is_string()) title = j["title"].get<std::string>().substr(0, 120);
            } catch (...) { return http::sendError(res, 400, "Invalid JSON body"); }
        }
        json d = ctx.threats.dashboard(user->id, false);
        json summary = {{"securityScore", d["securityScore"]}, {"securityLabel", d["securityLabel"]}, {"threatCount", d["threatCount"]},
                        {"criticalAlerts", d["criticalAlerts"]}, {"passwordStats", d["passwordStats"]}, {"urlStats", d["urlStats"]},
                        {"networkStats", d["networkStats"]}, {"fileStats", d["fileStats"]}, {"recommendations", ctx.threats.recommendations(d)}};
        int64_t id = ctx.db.insert("INSERT INTO reports(user_id,title,summary_json) VALUES(?,?,?)", {(long long)user->id, title, summary.dump()});
        ctx.logger.log("REPORT", "info", "Report generated: " + title, user->id, http::clientIp(req));
        http::sendJson(res, 201, {{"id", id}, {"title", title}, {"summary", summary}});
    });

    svr.Get("/api/admin/users", [&ctx](const httplib::Request& req, httplib::Response& res) {
        auto user = http::requireAuth(ctx, req, res, true);
        if (!user) return;
        http::sendJson(res, 200, {{"items", ctx.db.query("SELECT id, username, email, role, created_at, last_login FROM users ORDER BY id")}});
    });
}
