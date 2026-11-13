// Minimal dependency-free unit tests for the security engine.
#include <iostream>
#include <string>
#include "security/FileAnalyzer.h"
#include "security/PasswordAnalyzer.h"
#include "security/PortScanner.h"
#include "security/URLAnalyzer.h"
#include "utils/Crypto.h"

static int failures = 0;
#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) { std::cerr << "FAIL " << __LINE__ << ": " #cond "\n"; ++failures; } \
    } while (0)

int main() {
    // hashing (known vectors)
    CHECK(crypto::digestHex("abc", "SHA256") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(crypto::digestHex("abc", "SHA1") == "a9993e364706816aba3e25717850c26c9cd0d89d");
    CHECK(crypto::digestHex("abc", "MD5") == "900150983cd24fb0d6963f7d28e17f72");
    auto h = crypto::hashPassword("S3cret!Pass");
    CHECK(crypto::verifyPassword("S3cret!Pass", h));
    CHECK(!crypto::verifyPassword("wrong", h));
    CHECK(h.find("S3cret") == std::string::npos);

    // password analyzer
    PasswordAnalyzer pa;
    CHECK(pa.analyze("password").score < 30);
    CHECK(pa.analyze("12345678").strength == "Very Weak" || pa.analyze("12345678").score < 50);
    CHECK(pa.analyze("Tr0ub4dor&3xyz!").score >= 85);
    CHECK(pa.analyze("Password1!").score < 50);

    // URL analyzer
    URLAnalyzer ua;
    CHECK(ua.analyze("https://www.wikipedia.org/wiki/Cyber").riskLevel == "Low");
    auto bad = ua.analyze("http://paypal.com.secure-login.verify-account.xyz/signin?redirect=http://evil.tld");
    CHECK(bad.riskLevel == "High" || bad.riskLevel == "Critical");
    CHECK(ua.analyze("http://192.168.1.1/admin").riskScore >= 40);
    CHECK(ua.analyze("https://paypa1-support.com").riskScore >= 25);
    bool threw = false;
    try { ua.analyze("ftp://example.com"); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);

    // file analyzer
    FileAnalyzer fa;
    CHECK(fa.analyze("notes.txt", "hello world").verdict == "Clean");
    CHECK(fa.analyze("invoice.pdf.exe", std::string("MZ") + std::string(100, 'A')).verdict == "High Risk");
    CHECK(fa.analyze("t.com", "X5O!P%@AP EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*").verdict == "High Risk");

    // scanner safety limits
    PortScanner ps;
    bool blocked = false;
    try { ps.scan("8.8.8.8", 1, 10); } catch (const ScanPolicyError&) { blocked = true; } catch (...) { blocked = true; }
    CHECK(blocked);
    bool tooBig = false;
    try { ps.scan("127.0.0.1", 1, 5000); } catch (const std::invalid_argument&) { tooBig = true; }
    CHECK(tooBig);
    auto r = ps.scan("127.0.0.1", 1, 20);
    CHECK(r.ports.size() == 20);

    if (failures == 0) std::cout << "All tests passed\n";
    return failures == 0 ? 0 : 1;
}
