#include "api/Controllers.h"
#include "api/HttpUtil.h"

using nlohmann::json;

static json userJson(const AuthUser& u) { return {{"id", u.id}, {"username", u.username}, {"role", u.role}}; }

void registerAuthRoutes(httplib::Server& svr, ApiContext& ctx) {
    svr.Post("/api/auth/register", [&ctx](const httplib::Request& req, httplib::Response& res) {
        auto body = http::parseBody(req, res);
        if (!body) return;
        if (!body->value("username", json()).is_string() || !body->value("password", json()).is_string())
            return http::sendError(res, 400, "Fields 'username' and 'password' are required");
        try {
            auto user = ctx.auth.registerUser((*body)["username"], body->value("email", ""), (*body)["password"]);
            ctx.logger.log("REGISTER", "info", "New account created: " + user.username + " (" + user.role + ")", user.id, http::clientIp(req));
            http::sendJson(res, 201, {{"message", "Account created"}, {"user", userJson(user)}});
        } catch (const AuthError& e) {
            ctx.logger.log("REGISTER", "warning", std::string("Registration rejected: ") + e.what(), std::nullopt, http::clientIp(req));
            http::sendError(res, e.status(), e.what());
        }
    });

    svr.Post("/api/auth/login", [&ctx](const httplib::Request& req, httplib::Response& res) {
        auto body = http::parseBody(req, res);
        if (!body) return;
        if (!body->value("username", json()).is_string() || !body->value("password", json()).is_string())
            return http::sendError(res, 400, "Fields 'username' and 'password' are required");
        std::string username = (*body)["username"];
        try {
            auto r = ctx.auth.login(username, (*body)["password"], http::clientIp(req));
            ctx.logger.log("LOGIN_SUCCESS", "info", "User logged in: " + username, r.user.id, http::clientIp(req));
            http::sendJson(res, 200, {{"token", r.token}, {"expiresAt", r.expiresAt}, {"user", userJson(r.user)}});
        } catch (const AuthError& e) {
            ctx.logger.log("LOGIN_FAILED", e.status() == 429 ? "critical" : "warning",
                           std::string(e.status() == 429 ? "Account locked after repeated failures: " : "Failed login for: ") + username,
                           std::nullopt, http::clientIp(req));
            http::sendError(res, e.status(), e.what());
        }
    });

    svr.Get("/api/auth/me", [&ctx](const httplib::Request& req, httplib::Response& res) {
        auto user = http::requireAuth(ctx, req, res);
        if (!user) return;
        auto rows = ctx.db.query("SELECT created_at, last_login, email FROM users WHERE id = ?", {(long long)user->id});
        json u = userJson(*user);
        if (!rows.empty()) { u["createdAt"] = rows[0]["created_at"]; u["lastLogin"] = rows[0]["last_login"]; u["email"] = rows[0]["email"]; }
        http::sendJson(res, 200, {{"user", u}});
    });

    svr.Post("/api/auth/logout", [&ctx](const httplib::Request& req, httplib::Response& res) {
        auto user = http::requireAuth(ctx, req, res);
        if (!user) return;
        ctx.auth.logout(http::bearerToken(req));
        ctx.logger.log("LOGOUT", "info", "User logged out: " + user->username, user->id, http::clientIp(req));
        http::sendJson(res, 200, {{"message", "Logged out"}});
    });

    svr.Post("/api/auth/change-password", [&ctx](const httplib::Request& req, httplib::Response& res) {
        auto user = http::requireAuth(ctx, req, res);
        if (!user) return;
        auto body = http::parseBody(req, res);
        if (!body) return;
        if (!body->value("currentPassword", json()).is_string() || !body->value("newPassword", json()).is_string())
            return http::sendError(res, 400, "Fields 'currentPassword' and 'newPassword' are required");
        try {
            ctx.auth.changePassword(user->id, (*body)["currentPassword"], (*body)["newPassword"]);
            ctx.logger.log("PASSWORD_CHANGE", "info", "Password changed", user->id, http::clientIp(req));
            http::sendJson(res, 200, {{"message", "Password updated"}});
        } catch (const AuthError& e) {
            ctx.logger.log("PASSWORD_CHANGE", "warning", std::string("Password change rejected: ") + e.what(), user->id, http::clientIp(req));
            http::sendError(res, e.status(), e.what());
        }
    });
}
