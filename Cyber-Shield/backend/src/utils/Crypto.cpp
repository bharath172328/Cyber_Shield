#include "utils/Crypto.h"
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <vector>
#include "utils/StringUtil.h"

namespace crypto {
static const int kIterations = 600000;  // OWASP guidance for PBKDF2-HMAC-SHA256

std::string hexEncode(const unsigned char* d, size_t n) {
    static const char* h = "0123456789abcdef";
    std::string o;
    o.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) { o.push_back(h[d[i] >> 4]); o.push_back(h[d[i] & 15]); }
    return o;
}

static std::vector<unsigned char> hexDecode(const std::string& s) {
    if (s.size() % 2) throw std::invalid_argument("bad hex");
    std::vector<unsigned char> out;
    for (size_t i = 0; i < s.size(); i += 2) out.push_back((unsigned char)std::stoi(s.substr(i, 2), nullptr, 16));
    return out;
}

std::string digestHex(const std::string& data, const char* algo) {
    const EVP_MD* md = EVP_get_digestbyname(algo);
    if (!md) throw std::runtime_error("unknown digest");
    unsigned char out[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    if (EVP_Digest(data.data(), data.size(), out, &len, md, nullptr) != 1) throw std::runtime_error("digest failed");
    return hexEncode(out, len);
}

Digests hashAll(const std::string& data) {
    return {digestHex(data, "SHA256"), digestHex(data, "SHA1"), digestHex(data, "MD5")};
}

std::string randomHex(size_t bytes) {
    std::vector<unsigned char> buf(bytes);
    if (RAND_bytes(buf.data(), (int)bytes) != 1) throw std::runtime_error("RNG failure");
    return hexEncode(buf.data(), bytes);
}

bool constantTimeEquals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    return CRYPTO_memcmp(a.data(), b.data(), a.size()) == 0;
}

static std::string derive(const std::string& pw, const std::vector<unsigned char>& salt, int iters) {
    unsigned char out[32];
    if (PKCS5_PBKDF2_HMAC(pw.c_str(), (int)pw.size(), salt.data(), (int)salt.size(), iters, EVP_sha256(), 32, out) != 1)
        throw std::runtime_error("PBKDF2 failed");
    return hexEncode(out, 32);
}

std::string hashPassword(const std::string& password) {
    std::vector<unsigned char> salt(16);
    if (RAND_bytes(salt.data(), 16) != 1) throw std::runtime_error("RNG failure");
    return "pbkdf2_sha256$" + std::to_string(kIterations) + "$" + hexEncode(salt.data(), salt.size()) + "$" +
           derive(password, salt, kIterations);
}

bool verifyPassword(const std::string& password, const std::string& stored) {
    try {
        auto parts = util::split(stored, '$');
        if (parts.size() != 4 || parts[0] != "pbkdf2_sha256") return false;
        int iters = std::stoi(parts[1]);
        if (iters < 1000 || iters > 5000000) return false;
        return constantTimeEquals(derive(password, hexDecode(parts[2]), iters), parts[3]);
    } catch (...) {
        return false;
    }
}
}  // namespace crypto
