#include <csignal>
#include <iostream>
#include "api/ApiServer.h"

static ApiServer* g_server = nullptr;
static void onSignal(int) { if (g_server) g_server->stop(); }

int main() {
    try {
        Config config = Config::fromEnv();
        DatabaseManager db(config.dbPath);
        db.initSchema(config.schemaPath);
        SecurityLogger logger(db, config.logPath);
        AuthenticationManager auth(db, config.sessionTtlHours);
        auth.purgeExpired();

        PasswordAnalyzer passwords;
        URLAnalyzer urls;
        PortScanner scanner(config.scanAllowedHosts);
        FileHasher hasher;
        FileAnalyzer fileAnalyzer;
        FileIntegrityMonitor integrity(db);
        ThreatDetector threats(db);

        ApiContext ctx{config, db, logger, auth, passwords, urls, scanner, hasher, fileAnalyzer, integrity, threats};
        ApiServer server(ctx);
        g_server = &server;
        std::signal(SIGINT, onSignal);
        std::signal(SIGTERM, onSignal);

        if (!server.run()) {
            std::cerr << "Failed to bind port " << config.port << " (is it already in use?)" << std::endl;
            return 1;
        }
        logger.log("SYSTEM", "info", "Cyber Shield API stopped");
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return 1;
    }
}
