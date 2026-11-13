#include "security/FileHasher.h"
#include "utils/Crypto.h"

std::string FileHasher::sanitizeFilename(const std::string& raw) {
    std::string base = raw;
    auto slash = base.find_last_of("/\\");
    if (slash != std::string::npos) base = base.substr(slash + 1);
    std::string out;
    for (unsigned char c : base) out.push_back(c < 0x20 || c == 0x7f ? '_' : (char)c);
    if (out.size() > 255) out.resize(255);
    return out.empty() ? "unnamed" : out;
}

FileHashResult FileHasher::hash(const std::string& filename, const std::string& data) const {
    auto d = crypto::hashAll(data);
    return {sanitizeFilename(filename), data.size(), d.sha256, d.sha1, d.md5};
}
