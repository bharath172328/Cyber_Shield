#pragma once
#include <string>
#include <vector>

struct PasswordCheck {
    std::string name;
    bool passed;
    std::string detail;
};

struct PasswordResult {
    int score = 0;
    std::string strength;
    std::string message;
    double entropyBits = 0;
    std::vector<std::string> findings;
    std::vector<PasswordCheck> checks;
};

class PasswordAnalyzer {
public:
    PasswordResult analyze(const std::string& password) const;
private:
    static bool hasSequence(const std::string& s);
    static bool hasRepeats(const std::string& s);
};
