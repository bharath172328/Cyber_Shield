#include "security/FileAnalyzer.h"
#include <algorithm>
#include <cmath>
#include <set>
#include "utils/StringUtil.h"

namespace {
struct Type { std::string label, category; };

bool startsWith(const std::string& d, const std::string& p) { return d.size() >= p.size() && d.compare(0, p.size(), p) == 0; }

Type detect(const std::string& d) {
    if (startsWith(d, "MZ")) return {"Windows executable (PE)", "executable"};
    if (startsWith(d, "\x7f""ELF")) return {"Linux executable (ELF)", "executable"};
    if (startsWith(d, "\xcf\xfa\xed\xfe") || startsWith(d, "\xfe\xed\xfa\xcf") || startsWith(d, "\xca\xfe\xba\xbe")) return {"macOS executable (Mach-O)", "executable"};
    if (startsWith(d, "%PDF")) return {"PDF document", "document"};
    if (startsWith(d, "PK\x03\x04")) return {"ZIP-based archive / Office document", "archive"};
    if (startsWith(d, "\xd0\xcf\x11\xe0")) return {"Legacy Microsoft Office document", "document"};
    if (startsWith(d, "{\\rtf")) return {"RTF document", "document"};
    if (startsWith(d, "\x89PNG")) return {"PNG image", "image"};
    if (startsWith(d, "\xff\xd8\xff")) return {"JPEG image", "image"};
    if (startsWith(d, "GIF8")) return {"GIF image", "image"};
    if (startsWith(d, "\x1f\x8b")) return {"GZIP archive", "archive"};
    if (startsWith(d, "Rar!")) return {"RAR archive", "archive"};
    if (startsWith(d, "7z\xbc\xaf")) return {"7-Zip archive", "archive"};
    if (startsWith(d, "#!")) return {"Script (shebang)", "script"};
    size_t n = std::min<size_t>(d.size(), 4096);
    size_t printable = 0;
    for (size_t i = 0; i < n; ++i) { unsigned char c = d[i]; if (c == '\n' || c == '\r' || c == '\t' || (c >= 0x20 && c < 0x7f) || c >= 0x80) ++printable; }
    if (n > 0 && printable * 100 / n >= 95) return {"Plain text", "text"};
    if (d.empty()) return {"Empty file", "text"};
    return {"Unknown binary data", "unknown"};
}

double entropyOf(const std::string& d) {
    size_t n = std::min<size_t>(d.size(), 1 << 20);
    if (n == 0) return 0;
    size_t freq[256] = {0};
    for (size_t i = 0; i < n; ++i) ++freq[(unsigned char)d[i]];
    double e = 0;
    for (size_t f : freq) if (f) { double p = (double)f / n; e -= p * std::log2(p); }
    return std::round(e * 100) / 100;
}

std::string extensionOf(const std::string& name) {
    auto dot = name.rfind('.');
    return dot == std::string::npos ? "" : util::toLower(name.substr(dot + 1));
}
}  // namespace

FileAnalysisResult FileAnalyzer::analyze(const std::string& filename, const std::string& data) const {
    FileAnalysisResult r;
    Type t = detect(data);
    r.fileType = t.label;
    r.category = t.category;
    r.extension = extensionOf(filename);
    r.entropy = entropyOf(data);
    auto add = [&](const std::string& id, const std::string& desc, const std::string& sev, int w) {
        r.findings.push_back({id, desc, sev, w});
        r.riskScore += w;
    };

    static const std::set<std::string> kDangerous = {"exe", "dll", "scr", "bat", "cmd", "ps1", "vbs", "js", "jar", "msi", "com", "lnk", "hta", "sh"};
    static const std::set<std::string> kBenignExt = {"txt", "pdf", "jpg", "jpeg", "png", "gif", "doc", "docx", "xls", "xlsx", "csv", "md", "mp3", "mp4"};
    static const std::set<std::string> kExecExt = {"exe", "scr", "bat", "cmd", "js", "vbs", "ps1", "com", "jar"};

    std::string lname = util::toLower(filename);
    auto lastDot = lname.rfind('.');
    if (lastDot != std::string::npos && lastDot > 0) {
        auto prevDot = lname.rfind('.', lastDot - 1);
        if (prevDot != std::string::npos && kBenignExt.count(lname.substr(prevDot + 1, lastDot - prevDot - 1)) && kExecExt.count(r.extension))
            add("double_extension", "Double extension disguises an executable as a document", "high", 40);
    }
    if (kDangerous.count(r.extension)) add("dangerous_extension", "Extension '." + r.extension + "' can run code when opened", "medium", 15);
    if (t.category == "executable" && kBenignExt.count(r.extension))
        add("disguised_executable", "File is an executable but is named like a harmless ." + r.extension + " file", "high", 45);
    else if (t.category == "executable")
        add("executable", "File contains executable code; only run it if you trust the source", "low", 10);
    if (t.category == "image" && !r.extension.empty() && (r.extension == "png" || r.extension == "jpg" || r.extension == "jpeg" || r.extension == "gif") ) {
        bool match = (r.extension == "png" && t.label.find("PNG") == 0) || ((r.extension == "jpg" || r.extension == "jpeg") && t.label.find("JPEG") == 0) || (r.extension == "gif" && t.label.find("GIF") == 0);
        if (!match) add("type_mismatch", "Image extension does not match the actual image format", "low", 8);
    }
    if (r.entropy > 7.6 && data.size() > 1024 && t.category != "archive" && t.category != "image")
        add("high_entropy", "Very high entropy suggests packed or encrypted content (entropy " + std::to_string(r.entropy).substr(0, 4) + ")", "medium", 15);

    // signature / string scan over the first 2 MB
    std::string head = util::toLower(data.substr(0, 2u << 20));
    struct Sig { const char* needle; const char* id; const char* desc; const char* sev; int w; };
    static const Sig sigs[] = {
        {"eicar-standard-antivirus-test-file", "eicar", "Contains the EICAR antivirus test signature (harmless test file)", "high", 90},
        {"powershell -enc", "ps_encoded", "Encoded PowerShell command line", "high", 25},
        {"-encodedcommand", "ps_encoded2", "Encoded PowerShell command line", "high", 25},
        {"frombase64string", "b64_decode", "Decodes Base64 payloads at run time", "medium", 15},
        {"createremotethread", "injection", "References process-injection API calls", "high", 25},
        {"virtualallocex", "injection2", "References process-injection API calls", "high", 25},
        {"writeprocessmemory", "injection3", "References process-injection API calls", "high", 25},
        {"wscript.shell", "wscript", "Uses Windows Script Host shell object", "medium", 15},
        {"cmd.exe /c", "cmd", "Spawns a command shell", "medium", 10},
        {"/openaction", "pdf_openaction", "PDF runs an action automatically when opened", "medium", 15},
        {"/javascript", "pdf_js", "PDF contains embedded JavaScript", "medium", 15},
        {"/launch", "pdf_launch", "PDF can launch external programs", "high", 20},
        {"vbaproject.bin", "macro", "Document contains VBA macros", "medium", 20},
        {"autoopen", "macro_auto", "Macro that runs automatically on open", "high", 20},
        {"document_open", "macro_auto2", "Macro that runs automatically on open", "high", 20},
        {"unescape(", "js_obf", "Obfuscated JavaScript pattern", "medium", 10},
    };
    std::set<std::string> seenDesc;
    for (const auto& s : sigs) {
        if (head.find(s.needle) != std::string::npos && seenDesc.insert(s.desc).second) add(s.id, s.desc, s.sev, s.w);
    }

    r.riskScore = std::min(100, r.riskScore);
    r.verdict = r.riskScore >= 45 ? "High Risk" : (r.riskScore >= 15 ? "Suspicious" : "Clean");
    return r;
}
