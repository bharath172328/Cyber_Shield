#include "security/URLAnalyzer.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif
#include <algorithm>
#include <cctype>
#include <set>
#include <stdexcept>
#include "utils/StringUtil.h"

using util::contains;

namespace {
const std::set<std::string> kShorteners = {"bit.ly", "tinyurl.com", "t.co", "goo.gl", "is.gd", "ow.ly", "buff.ly", "rebrand.ly", "cutt.ly"};
const std::set<std::string> kRiskyTlds = {"zip", "xyz", "top", "tk", "ml", "ga", "cf", "gq", "click", "work", "support", "country", "kim", "loan", "mov"};
const std::set<std::string> kTwoPartTlds = {"co.uk", "org.uk", "co.in", "com.au", "co.jp", "com.br", "co.za"};
const char* kBrands[] = {"paypal", "apple", "google", "microsoft", "amazon", "facebook", "netflix", "instagram",
                         "whatsapp", "linkedin", "dropbox", "github", "chase", "wellsfargo", "hdfc", "icici", "sbi"};
const char* kKeywords[] = {"login", "signin", "verify", "secure", "account", "update", "confirm", "banking",
                           "password", "wallet", "invoice", "suspend", "unlock", "recover"};

bool isIPv4(const std::string& h) {
    in_addr a{};
    return inet_pton(AF_INET, h.c_str(), &a) == 1;
}
bool allDigits(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
}
// Undo common look-alike substitutions (paypa1 -> paypal).
std::string deLeet(std::string s) {
    for (char& c : s) {
        switch (c) { case '0': c = 'o'; break; case '1': c = 'l'; break; case '3': c = 'e'; break;
                     case '5': c = 's'; break; case '4': c = 'a'; break; default: break; }
    }
    return s;
}
}  // namespace

UrlResult URLAnalyzer::analyze(const std::string& input) const {
    std::string url = util::trim(input);
    if (url.empty() || url.size() > 2048) throw std::invalid_argument("URL must be 1-2048 characters");
    for (unsigned char c : url)
        if (c <= 0x20 || c == 0x7f) throw std::invalid_argument("URL must not contain spaces or control characters");

    UrlResult r;
    r.url = url;
    auto add = [&](const std::string& id, const std::string& desc, const std::string& sev, int w) {
        r.indicators.push_back({id, desc, sev, w});
        r.riskScore += w;
    };

    // ---- parse scheme://[userinfo@]host[:port]/path?query#fragment
    std::string rest = url;
    auto sp = rest.find("://");
    bool schemeMissing = sp == std::string::npos;
    if (schemeMissing) r.scheme = "http";
    else {
        r.scheme = util::toLower(rest.substr(0, sp));
        rest = rest.substr(sp + 3);
    }
    if (r.scheme != "http" && r.scheme != "https") throw std::invalid_argument("Only http and https URLs can be analysed");

    auto endAuth = rest.find_first_of("/?#");
    std::string authority = rest.substr(0, endAuth);
    std::string tail = endAuth == std::string::npos ? "" : rest.substr(endAuth);
    bool hasUserinfo = false;
    if (auto at = authority.rfind('@'); at != std::string::npos) {
        hasUserinfo = true;
        authority = authority.substr(at + 1);
    }
    std::string host = authority;
    r.port = r.scheme == "https" ? 443 : 80;
    bool explicitPort = false;
    bool ipv6 = !authority.empty() && authority[0] == '[';
    if (!ipv6) {
        if (auto c = authority.rfind(':'); c != std::string::npos) {
            host = authority.substr(0, c);
            std::string p = authority.substr(c + 1);
            if (!allDigits(p) || p.size() > 5 || std::stoi(p) > 65535) throw std::invalid_argument("Invalid port in URL");
            r.port = std::stoi(p);
            explicitPort = true;
        }
    }
    host = util::toLower(host);
    if (host.empty()) throw std::invalid_argument("URL has no host");
    r.host = host;
    auto pathEnd = tail.find_first_of("?#");
    r.path = tail.empty() ? "/" : tail.substr(0, pathEnd);
    if (r.path.empty()) r.path = "/";
    std::string lowerTail = util::toLower(tail);

    // ---- indicators
    if (r.scheme != "https") add("no_https", "Connection is not encrypted (HTTP instead of HTTPS)" + std::string(schemeMissing ? " - no scheme given, HTTP assumed" : ""), "medium", 15);
    if (hasUserinfo) add("userinfo", "URL contains '@' credentials section, often used to disguise the real destination", "high", 25);

    bool ipHost = ipv6 || isIPv4(host);
    bool obfuscatedIp = !ipHost && (allDigits(host) || (host.rfind("0x", 0) == 0));
    if (ipHost) add("ip_host", "Host is a raw IP address instead of a domain name", "high", 25);
    if (obfuscatedIp) add("obfuscated_ip", "Host looks like an encoded (decimal/hex) IP address", "high", 30);

    std::vector<std::string> labels = util::split(host, '.');
    std::string sld, tld = labels.back();
    size_t subdomains = 0;
    if (!ipHost && !obfuscatedIp) {
        size_t n = labels.size();
        std::string last2 = n >= 2 ? labels[n - 2] + "." + labels[n - 1] : "";
        size_t regLabels = kTwoPartTlds.count(last2) ? 3 : 2;
        if (n >= regLabels) { sld = labels[n - regLabels]; subdomains = n - regLabels; }
        else sld = labels[0];

        if (subdomains > 3) add("many_subdomains", "Excessive number of subdomains (" + std::to_string(subdomains) + ")", "medium", 15);
        else if (subdomains == 3) add("several_subdomains", "Unusually deep subdomain structure", "low", 6);
        if (kRiskyTlds.count(tld)) add("risky_tld", "Top-level domain '." + tld + "' is frequently abused", "medium", 10);
        if (contains(host, "xn--")) add("punycode", "Internationalised (punycode) domain - possible look-alike characters", "high", 15);
        int hyphens = (int)std::count(host.begin(), host.end(), '-');
        if (hyphens >= 3) add("many_hyphens", "Domain contains many hyphens", "low", 8);
        int digits = (int)std::count_if(sld.begin(), sld.end(), [](unsigned char c) { return std::isdigit(c); });
        if (sld.size() >= 6 && digits * 3 >= (int)sld.size()) add("digit_heavy", "Domain name has an unusually high share of digits", "low", 8);
        if (kShorteners.count(host)) add("shortener", "URL shortener hides the final destination", "medium", 15);

        // brand impersonation & look-alikes
        for (const char* b : kBrands) {
            std::string brand = b;
            if (sld == brand) continue;
            bool flagged = false;
            for (const auto& lab : labels)
                for (const auto& tok : util::split(lab, '-'))
                    if (tok == brand) flagged = true;
            if (flagged) { add("brand_impersonation", "Contains the brand '" + brand + "' but is not the brand's own domain", "high", 25); break; }
            for (const auto& tok : util::split(sld, '-'))
                if (tok != brand && deLeet(tok) == brand) { add("lookalike", "Domain '" + sld + "' imitates '" + brand + "' using look-alike characters", "high", 25); flagged = true; break; }
            if (flagged) break;
        }
    }

    if (url.size() > 150) add("very_long", "URL is extremely long (" + std::to_string(url.size()) + " characters)", "medium", 15);
    else if (url.size() > 75) add("long", "URL is longer than typical (" + std::to_string(url.size()) + " characters)", "low", 8);

    int hits = 0;
    for (const char* k : kKeywords) if (contains(host, k) || contains(lowerTail, k)) ++hits;
    if (hits > 0) add("keywords", "Contains " + std::to_string(hits) + " phishing-style keyword(s) such as login, verify or account", hits >= 3 ? "high" : "medium", std::min(18, hits * 6));

    int encoded = 0;
    for (size_t i = 0; i + 2 < url.size(); ++i) if (url[i] == '%' && std::isxdigit((unsigned char)url[i + 1]) && std::isxdigit((unsigned char)url[i + 2])) ++encoded;
    if (encoded > 3) add("encoding", "Heavy percent-encoding can hide the true path or parameters", "medium", 10);
    if (std::any_of(url.begin(), url.end(), [](unsigned char c) { return c >= 0x80; })) add("non_ascii", "URL contains non-ASCII characters", "medium", 12);
    if (explicitPort && r.port != 80 && r.port != 443) add("odd_port", "Uses a non-standard port (" + std::to_string(r.port) + ")", "medium", 10);
    if (contains(lowerTail, "redirect=http") || contains(lowerTail, "url=http") || contains(lowerTail, "next=http"))
        add("redirect_param", "Query string contains an embedded redirect to another site", "medium", 10);
    if (r.path.find("//") != std::string::npos) add("double_slash", "Path contains a double slash", "low", 4);

    r.riskScore = std::min(100, r.riskScore);
    if (r.riskScore < 20) { r.riskLevel = "Low"; r.recommendation = "No strong phishing indicators found. Still verify the sender before entering credentials."; }
    else if (r.riskScore < 45) { r.riskLevel = "Medium"; r.recommendation = "Some suspicious traits detected. Do not enter passwords; confirm the link through an official channel."; }
    else if (r.riskScore < 70) { r.riskLevel = "High"; r.recommendation = "Likely phishing or malicious. Avoid opening this link and report it to your security team."; }
    else { r.riskLevel = "Critical"; r.recommendation = "Multiple strong phishing indicators. Do not visit; block the URL and report it."; }
    return r;
}
