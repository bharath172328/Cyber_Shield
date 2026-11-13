#pragma once
#include <string>
#include <vector>

struct FileFinding {
    std::string id, description, severity;
    int weight;
};

struct FileAnalysisResult {
    std::string fileType, category, extension;
    double entropy = 0;
    int riskScore = 0;
    std::string verdict;  // Clean | Suspicious | High Risk
    std::vector<FileFinding> findings;
};

// Static, read-only inspection of file bytes. Files are never executed.
class FileAnalyzer {
public:
    FileAnalysisResult analyze(const std::string& filename, const std::string& data) const;
};
