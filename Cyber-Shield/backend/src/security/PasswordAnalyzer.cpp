#include "security/PasswordAnalyzer.h"
#include <algorithm>
#include <cmath>
#include "utils/StringUtil.h"

static const char* kCommon[] = {"password", "passw0rd", "123456", "qwerty", "admin", "letmein", "welcome",
                                "iloveyou", "abc123", "monkey", "dragon", "football", "login", "master"};

bool PasswordAnalyzer::hasSequence(const std::string& s) {
    for (size_t i = 0; i + 2 < s.size(); ++i) {
        int a = std::tolower((unsigned char)s[i]), b = std::tolower((unsigned char)s[i + 1]),
            c = std::tolower((unsigned char)s[i + 2]);
        if (!std::isalnum(a) || !std::isalnum(b) || !std::isalnum(c)) continue;
        if ((b == a + 1 && c == b + 1) || (b == a - 1 && c == b - 1)) return true;
    }
    return false;
}

bool PasswordAnalyzer::hasRepeats(const std::string& s) {
    for (size_t i = 0; i + 2 < s.size(); ++i)
        if (s[i] == s[i + 1] && s[i] == s[i + 2]) return true;
    return false;
}

PasswordResult PasswordAnalyzer::analyze(const std::string& pw) const {
    PasswordResult r;
    bool up = false, low = false, dig = false, sym = false;
    for (unsigned char c : pw) {
        if (std::isupper(c)) up = true;
        else if (std::islower(c)) low = true;
        else if (std::isdigit(c)) dig = true;
        else sym = true;
    }
    const size_t len = pw.size();
    const bool common = [&] {
        std::string l = util::toLower(pw);
        for (const char* w : kCommon) if (l.find(w) != std::string::npos) return true;
        return false;
    }();
    const bool seq = hasSequence(pw), rep = hasRepeats(pw);

    int score = std::min<int>(40, (int)len * 3);
    if (len >= 12) score += 10;
    if (up) score += 10;
    if (low) score += 10;
    if (dig) score += 15;
    if (sym) score += 15;
    if (seq) score -= 10;
    if (rep) score -= 10;
    if (common) score = std::min(score - 20, 35);  // common words can never rate above "Weak"
    if (len < 8) score = std::min(score, 40);
    r.score = std::clamp(score, 0, 100);

    int pool = (up ? 26 : 0) + (low ? 26 : 0) + (dig ? 10 : 0) + (sym ? 33 : 0);
    r.entropyBits = pool ? std::round(len * std::log2((double)pool) * 10) / 10 : 0;

    r.checks = {
        {"Length", len >= 12 ? true : len >= 8, std::to_string(len) + " characters (12+ recommended)"},
        {"Uppercase letters", up, up ? "Present" : "Add A-Z"},
        {"Lowercase letters", low, low ? "Present" : "Add a-z"},
        {"Numbers", dig, dig ? "Present" : "Add 0-9"},
        {"Special characters", sym, sym ? "Present" : "Add symbols such as ! @ # $"},
        {"No repeated characters", !rep, rep ? "Three or more identical characters in a row" : "None found"},
        {"No sequences", !seq, seq ? "Sequential run such as abc or 123" : "None found"},
        {"Not a common password", !common, common ? "Contains a widely used weak pattern" : "None found"},
    };
    for (auto& c : r.checks)
        if (!c.passed) r.findings.push_back(c.name + ": " + c.detail);

    if (r.score < 30) { r.strength = "Very Weak"; r.message = "Password is easy to guess"; }
    else if (r.score < 50) { r.strength = "Weak"; r.message = "Password needs improvement"; }
    else if (r.score < 70) { r.strength = "Moderate"; r.message = "Password is acceptable but could be stronger"; }
    else if (r.score < 85) { r.strength = "Strong"; r.message = "Password has good security"; }
    else { r.strength = "Very Strong"; r.message = "Password has excellent security"; }
    return r;
}
