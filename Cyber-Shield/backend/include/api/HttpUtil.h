#pragma once
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include "api/ApiContext.h"

namespace http {
void sendJson(httplib::Response& res, int status, const nlohmann::json& body);
void sendError(httplib::Response& res, int status, const std::string& message);
std::optional<nlohmann::json> parseBody(const httplib::Request& req, httplib::Response& res);
std::string bearerToken(const httplib::Request& req);
// Returns the authenticated user, or writes a 401/403 response and returns nullopt.
std::optional<AuthUser> requireAuth(ApiContext& ctx, const httplib::Request& req, httplib::Response& res, bool adminOnly = false);
std::string clientIp(const httplib::Request& req);
int queryInt(const httplib::Request& req, const std::string& key, int def, int lo, int hi);
}  // namespace http
