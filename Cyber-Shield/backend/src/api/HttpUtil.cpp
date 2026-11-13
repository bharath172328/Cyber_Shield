#include "api/HttpUtil.h"
#include <algorithm>
#include <cstdlib>

namespace http {
using nlohmann::json;

void sendJson(httplib::Response& res, int status, const json& body) {
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

void sendError(httplib::Response& res, int status, const std::string& message) {
    sendJson(res, status, {{"error", message}, {"status", status}});
}

std::optional<json> parseBody(const httplib::Request& req, httplib::Response& res) {
    if (req.body.empty()) { sendError(res, 400, "Request body is required"); return std::nullopt; }
    try {
        json j = json::parse(req.body);
        if (!j.is_object()) { sendError(res, 400, "Request body must be a JSON object"); return std::nullopt; }
        return j;
    } catch (const json::exception&) {
        sendError(res, 400, "Invalid JSON body");
        return std::nullopt;
    }
}

std::string bearerToken(const httplib::Request& req) {
    std::string h = req.get_header_value("Authorization");
    const std::string prefix = "Bearer ";
    return h.rfind(prefix, 0) == 0 ? h.substr(prefix.size()) : "";
}

std::optional<AuthUser> requireAuth(ApiContext& ctx, const httplib::Request& req, httplib::Response& res, bool adminOnly) {
    auto user = ctx.auth.authenticate(bearerToken(req));
    if (!user) { sendError(res, 401, "Authentication required"); return std::nullopt; }
    if (adminOnly && user->role != "admin") {
        ctx.logger.log("SECURITY_EVENT", "warning", "Non-admin attempted admin action: " + req.path, user->id, clientIp(req));
        sendError(res, 403, "Administrator access required");
        return std::nullopt;
    }
    return user;
}

std::string clientIp(const httplib::Request& req) { return req.remote_addr; }

int queryInt(const httplib::Request& req, const std::string& key, int def, int lo, int hi) {
    if (!req.has_param(key)) return def;
    try { return std::clamp(std::stoi(req.get_param_value(key)), lo, hi); } catch (...) { return def; }
}
}  // namespace http
