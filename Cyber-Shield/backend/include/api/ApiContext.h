#pragma once
#include "authentication/AuthenticationManager.h"
#include "database/DatabaseManager.h"
#include "logging/SecurityLogger.h"
#include "security/FileAnalyzer.h"
#include "security/FileHasher.h"
#include "security/FileIntegrityMonitor.h"
#include "security/PasswordAnalyzer.h"
#include "security/PortScanner.h"
#include "security/ThreatDetector.h"
#include "security/URLAnalyzer.h"
#include "utils/Config.h"

// Bundle of shared services handed to every controller.
struct ApiContext {
    Config& config;
    DatabaseManager& db;
    SecurityLogger& logger;
    AuthenticationManager& auth;
    PasswordAnalyzer& passwords;
    URLAnalyzer& urls;
    PortScanner& scanner;
    FileHasher& hasher;
    FileAnalyzer& fileAnalyzer;
    FileIntegrityMonitor& integrity;
    ThreatDetector& threats;
};
