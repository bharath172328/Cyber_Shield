#include "api/ApiServer.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include "api/Controllers.h"
#include "api/HttpUtil.h"

using nlohmann::json;

ApiServer::ApiServer(ApiContext& ctx) : ctx_(ctx) { configure(); }

void ApiServer::configure() {
    svr_.set_payload_max_length(ctx_.config.maxUploadBytes);
    svr_.set_read_timeout(30, 0);
    svr_.set_write_timeout(30, 0);

    // CORS + security headers on every response; answer pre-flight requests immediately.
    svr_.set_pre_routing_handler([this](const httplib::Request& req, httplib::Response& res) {
        std::string origin = req.get_header_value("Origin");
        const auto& allowed = ctx_.config.corsOrigins;
        if (!origin.empty() && std::find(allowed.begin(), allowed.end(), origin) != allowed.end()) {
            res.set_header("Access-Control-Allow-Origin", origin);
            res.set_header("Vary", "Origin");
            res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
            res.set_header("Access-Control-Max-Age", "600");
        }
        res.set_header("X-Content-Type-Options", "nosniff");
        res.set_header("Cache-Control", "no-store");
        if (req.method == "OPTIONS") { res.status = 204; return httplib::Server::HandlerResponse::Handled; }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    svr_.set_exception_handler([this](const httplib::Request& req, httplib::Response& res, std::exception_ptr ep) {
        std::string what = "unknown";
        try { if (ep) std::rethrow_exception(ep); } catch (const std::exception& e) { what = e.what(); } catch (...) {}
        ctx_.logger.log("ERROR", "error", "Unhandled exception on " + req.method + " " + req.path + ": " + what, std::nullopt, req.remote_addr);
        http::sendError(res, 500, "Internal server error");
    });

    // Serve the built React dashboard (frontend/dist) from the same port as the API.
    namespace fs = std::filesystem;
    const fs::path distDir = fs::absolute(ctx_.config.frontendDir);
    const fs::path indexFile = distDir / "index.html";
    hasFrontend_ = fs::exists(indexFile);
    if (hasFrontend_) {
        svr_.set_mount_point("/", distDir.string());
        std::cout << "Serving dashboard from " << distDir.string() << std::endl;
    } else {
        std::cout << "Dashboard build not found at " << distDir.string()
                  << " - run 'npm install && npm run build' in frontend/ (or use 'npm run dev')." << std::endl;
    }

    const bool hasFrontend = hasFrontend_;
    svr_.set_error_handler([hasFrontend, indexFile](const httplib::Request& req, httplib::Response& res) {
        const bool isApi = req.path.rfind("/api", 0) == 0;
        if (res.status == 404 && !isApi && req.method == "GET") {
            if (hasFrontend) {
                // Single-page-app fallback: /dashboard, /login ... all load index.html
                std::ifstream in(indexFile, std::ios::binary);
                std::stringstream ss; ss << in.rdbuf();
                res.status = 200;
                res.set_content(ss.str(), "text/html; charset=utf-8");
                return;
            }
            res.status = 200;
            res.set_content(
                "<!doctype html><meta charset=utf-8><title>Cyber Shield API</title>"
                "<body style=\"font-family:sans-serif;background:#070b13;color:#e2e8f0;padding:48px;max-width:640px;margin:auto\">"
                "<h1>&#128737; Cyber Shield API is running</h1>"
                "<p>The backend is healthy (<a style=\"color:#38bdf8\" href=\"/api/health\">/api/health</a>), "
                "but the dashboard has not been built yet.</p>"
                "<p><b>Option 1 (development):</b><br><code>cd frontend &amp;&amp; npm install &amp;&amp; npm run dev</code> "
                "then open <a style=\"color:#38bdf8\" href=\"http://localhost:5173\">http://localhost:5173</a></p>"
                "<p><b>Option 2 (served from this port):</b><br><code>cd frontend &amp;&amp; npm install &amp;&amp; npm run build</code>, "
                "restart the backend and open this page again.</p></body>", "text/html; charset=utf-8");
            return;
        }
        if (res.body.empty()) {
            int s = res.status;
            std::string msg = s == 404 ? "Endpoint not found" : (s == 405 ? "Method not allowed" : (s == 413 ? "Upload too large" : "Request failed"));
            http::sendError(res, s, msg);
        }
    });

    svr_.Get("/api/health", [this](const httplib::Request&, httplib::Response& res) {
        json j = {{"status", "ok"}, {"service", "cyber-shield"}, {"version", "1.0.0"}};
        try {
            auto r = ctx_.db.query("SELECT COUNT(*) AS c FROM users");
            j["database"] = {{"status", "ok"}, {"engine", "sqlite " + ctx_.db.sqliteVersion()}, {"users", r[0]["c"]}};
        } catch (const std::exception& e) {
            j["status"] = "degraded";
            j["database"] = {{"status", "error"}};
            return http::sendJson(res, 503, j);
        }
        http::sendJson(res, 200, j);
    });

    registerAuthRoutes(svr_, ctx_);
    registerSecurityRoutes(svr_, ctx_);
    registerDashboardRoutes(svr_, ctx_);
}

bool ApiServer::run() {
    ctx_.logger.log("SYSTEM", "info", "Cyber Shield API starting on port " + std::to_string(ctx_.config.port));
    return svr_.listen("0.0.0.0", ctx_.config.port);
}

void ApiServer::stop() { svr_.stop(); }
