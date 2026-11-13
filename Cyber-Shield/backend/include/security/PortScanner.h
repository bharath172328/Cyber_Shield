#pragma once
#include <stdexcept>
#include <string>
#include <vector>

struct ScanPortResult {
    int port;
    std::string state;    // open | closed | filtered
    std::string service;  // well-known service name for the port number (no banner grabbing)
    std::string risk;     // none | low | medium | high
    std::string note;
};

struct ScanResult {
    std::string host, resolvedIp;
    int startPort = 0, endPort = 0;
    long durationMs = 0;
    std::vector<ScanPortResult> ports;
    int openCount = 0, closedCount = 0, filteredCount = 0, riskyOpenCount = 0;
};

class ScanPolicyError : public std::runtime_error { public: using std::runtime_error::runtime_error; };
class ScanBusyError : public std::runtime_error { public: using std::runtime_error::runtime_error; };

// Defensive TCP connect scanner with hard safety limits:
//  * only loopback / private (RFC1918) addresses, or hosts the operator explicitly allow-lists
//  * at most kMaxPorts ports per scan, one scan at a time, bounded threads and timeouts
class PortScanner {
public:
    static constexpr int kMaxPorts = 1024;
    explicit PortScanner(std::vector<std::string> allowedHosts = {});
    ScanResult scan(const std::string& host, int startPort, int endPort, int timeoutMs = 400) const;

private:
    std::vector<std::string> allowed_;
};
