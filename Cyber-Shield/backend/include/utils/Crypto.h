#pragma once
#include <string>

namespace crypto {
struct Digests { std::string sha256, sha1, md5; };

std::string hexEncode(const unsigned char* data, size_t n);
std::string digestHex(const std::string& data, const char* algo);  // "SHA256" | "SHA1" | "MD5"
Digests hashAll(const std::string& data);
std::string randomHex(size_t bytes);

// PBKDF2-HMAC-SHA256 with a random per-password salt.
std::string hashPassword(const std::string& password);
bool verifyPassword(const std::string& password, const std::string& stored);
bool constantTimeEquals(const std::string& a, const std::string& b);
}  // namespace crypto
