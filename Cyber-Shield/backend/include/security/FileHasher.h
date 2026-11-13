#pragma once
#include <cstddef>
#include <string>

struct FileHashResult {
    std::string filename;
    size_t size = 0;
    std::string sha256, sha1, md5;
};

class FileHasher {
public:
    FileHashResult hash(const std::string& filename, const std::string& data) const;
    // Strip directory parts and control characters from an uploaded file name.
    static std::string sanitizeFilename(const std::string& raw);
};
