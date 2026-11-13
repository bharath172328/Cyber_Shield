#pragma once
#include <string>
#include <vector>

struct UrlIndicator {
    std::string id;
    std::string description;
    std::string severity;  // low | medium | high
    int weight;
};

struct UrlResult {
    std::string url, scheme, host, path;
    int port = 0;
    int riskScore = 0;
    std::string riskLevel;  // Low | Medium | High | Critical
    std::vector<UrlIndicator> indicators;
    std::string recommendation;
};

// Purely static/lexical analysis. It never contacts the target URL.
class URLAnalyzer {
public:
    // Throws std::invalid_argument when the input cannot be parsed as a URL.
    UrlResult analyze(const std::string& url) const;
};
