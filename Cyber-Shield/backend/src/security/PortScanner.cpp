#include "security/PortScanner.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#define close closesocket
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <map>
#include <thread>
#include "utils/StringUtil.h"

namespace {
std::atomic<bool> g_busy{false};

struct PortInfo { const char* service; const char* risk; const char* note; };
const std::map<int, PortInfo> kPorts = {
    {21, {"FTP", "high", "Unencrypted file transfer; credentials sent in clear text"}},
    {22, {"SSH", "low", "Encrypted remote shell; restrict by key and firewall"}},
    {23, {"Telnet", "high", "Unencrypted remote shell; replace with SSH"}},
    {25, {"SMTP", "medium", "Mail relay; ensure it is not an open relay"}},
    {53, {"DNS", "low", "Name service; restrict recursion"}},
    {80, {"HTTP", "low", "Unencrypted web traffic; redirect to HTTPS"}},
    {110, {"POP3", "medium", "Unencrypted mail retrieval"}},
    {135, {"MS-RPC", "high", "Windows RPC; should not be exposed"}},
    {139, {"NetBIOS", "high", "Legacy Windows file sharing"}},
    {143, {"IMAP", "medium", "Unencrypted mail access"}},
    {443, {"HTTPS", "none", "Encrypted web traffic"}},
    {445, {"SMB", "high", "Windows file sharing; frequent ransomware entry point"}},
    {993, {"IMAPS", "none", "Encrypted mail access"}},
    {995, {"POP3S", "none", "Encrypted mail retrieval"}},
    {1433, {"MSSQL", "high", "Database should not be network-exposed"}},
    {3306, {"MySQL", "high", "Database should not be network-exposed"}},
    {3389, {"RDP", "high", "Remote Desktop; heavily targeted by brute force"}},
    {5432, {"PostgreSQL", "high", "Database should not be network-exposed"}},
    {5900, {"VNC", "high", "Remote desktop, often weakly protected"}},
    {6379, {"Redis", "high", "Frequently deployed without authentication"}},
    {8080, {"HTTP-alt", "medium", "Alternate web port; often admin consoles or dev servers"}},
    {8443, {"HTTPS-alt", "low", "Alternate encrypted web port"}},
    {9200, {"Elasticsearch", "high", "Often deployed without authentication"}},
    {11211, {"Memcached", "high", "Should not be network-exposed"}},
    {27017, {"MongoDB", "high", "Database should not be network-exposed"}},
};

bool isSafeRange(uint32_t ip) {  // ip in host byte order
    if ((ip >> 24) == 127) return true;                       // 127.0.0.0/8
    if ((ip >> 24) == 10) return true;                        // 10.0.0.0/8
    if ((ip >> 20) == ((172u << 4) | 1)) return true;         // 172.16.0.0/12
    if ((ip >> 16) == ((192u << 8) | 168)) return true;       // 192.168.0.0/16
    return false;
}

std::string probe(const sockaddr_in& addr, int timeoutMs) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return "filtered";
    std::string state = "filtered";
#ifdef _WIN32
    u_long mode = 1;
    if (ioctlsocket(fd, FIONBIO, &mode) != 0) {
        closesocket(fd);
        return state;
    }
    int rc = connect(fd, (const sockaddr*)&addr, sizeof addr);
    if (rc == 0) state = "open";
    else if (WSAGetLastError() == WSAECONNREFUSED) state = "closed";
    else if (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINVAL) {
        fd_set wset;
        FD_ZERO(&wset);
        FD_SET(fd, &wset);
        timeval tv{};
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;
        if (select(0, nullptr, &wset, nullptr, &tv) > 0) {
            int err = 0;
            int len = sizeof err;
            getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
            state = err == 0 ? "open" : (err == WSAECONNREFUSED ? "closed" : "filtered");
        }
    }
#else
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    int rc = connect(fd, (const sockaddr*)&addr, sizeof addr);
    if (rc == 0) state = "open";
    else if (errno == ECONNREFUSED) state = "closed";
    else if (errno == EINPROGRESS) {
        pollfd p{fd, POLLOUT, 0};
        if (poll(&p, 1, timeoutMs) > 0) {
            int err = 0;
            socklen_t len = sizeof err;
            getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
            state = err == 0 ? "open" : (err == ECONNREFUSED ? "closed" : "filtered");
        }
    }
#endif
    close(fd);
    return state;
}
}  // namespace

PortScanner::PortScanner(std::vector<std::string> allowedHosts) : allowed_(std::move(allowedHosts)) {}

ScanResult PortScanner::scan(const std::string& host, int startPort, int endPort, int timeoutMs) const {
    if (host.empty() || host.size() > 253) throw std::invalid_argument("Host must be 1-253 characters");
    for (unsigned char c : host)
        if (!(std::isalnum(c) || c == '.' || c == '-')) throw std::invalid_argument("Host contains invalid characters");
    if (startPort < 1 || endPort > 65535 || startPort > endPort) throw std::invalid_argument("Port range must be within 1-65535 and start <= end");
    if (endPort - startPort + 1 > kMaxPorts)
        throw std::invalid_argument("Range too large: at most " + std::to_string(kMaxPorts) + " ports per scan");
    timeoutMs = std::clamp(timeoutMs, 100, 2000);
#ifdef _WIN32
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) throw std::runtime_error("WinSock initialization failed");
    struct WsaGuard { ~WsaGuard() { WSACleanup(); } } wsaGuard;
#endif

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* res = nullptr;
    if (getaddrinfo(host.c_str(), nullptr, &hints, &res) != 0 || !res) throw std::invalid_argument("Host could not be resolved");
    sockaddr_in target = *(sockaddr_in*)res->ai_addr;
    freeaddrinfo(res);
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &target.sin_addr, ipStr, sizeof ipStr);
    uint32_t ipHost = ntohl(target.sin_addr.s_addr);

    bool allowListed = std::any_of(allowed_.begin(), allowed_.end(), [&](const std::string& a) {
        return util::toLower(a) == util::toLower(host) || a == ipStr;
    });
    if (!isSafeRange(ipHost) && !allowListed)
        throw ScanPolicyError("Scanning is limited to loopback and private-network addresses. Ask the administrator to allow-list " + host + " if you are authorised to test it");

    bool expected = false;
    if (!g_busy.compare_exchange_strong(expected, true)) throw ScanBusyError("Another scan is already running. Try again shortly");
    struct Release { ~Release() { g_busy = false; } } release;

    ScanResult r;
    r.host = host;
    r.resolvedIp = ipStr;
    r.startPort = startPort;
    r.endPort = endPort;
    const int total = endPort - startPort + 1;
    r.ports.resize(total);

    auto t0 = std::chrono::steady_clock::now();
    std::atomic<int> next{0};
    auto worker = [&] {
        for (int i; (i = next.fetch_add(1)) < total;) {
            int port = startPort + i;
            sockaddr_in a = target;
            a.sin_port = htons((uint16_t)port);
            std::string state = probe(a, timeoutMs);
            ScanPortResult pr{port, state, "unknown", "none", ""};
            auto it = kPorts.find(port);
            if (it != kPorts.end()) {
                pr.service = it->second.service;
                if (state == "open") { pr.risk = it->second.risk; pr.note = it->second.note; }
            } else if (state == "open") {
                pr.risk = "low";
                pr.note = "Non-standard service; confirm it is intended to be exposed";
            }
            r.ports[i] = std::move(pr);
        }
    };
    int threads = std::min(64, total);
    std::vector<std::thread> pool;
    for (int i = 0; i < threads; ++i) pool.emplace_back(worker);
    for (auto& t : pool) t.join();
    r.durationMs = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();

    for (auto& p : r.ports) {
        if (p.state == "open") { ++r.openCount; if (p.risk == "high" || p.risk == "medium") ++r.riskyOpenCount; }
        else if (p.state == "closed") ++r.closedCount;
        else ++r.filteredCount;
    }
    return r;
}
