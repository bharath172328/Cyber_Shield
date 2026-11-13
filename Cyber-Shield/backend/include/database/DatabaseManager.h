#pragma once
#include <sqlite3.h>
#include <cstdint>
#include <mutex>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

// A bound SQL parameter. Implicit constructors keep call sites tidy:
//   db.query("SELECT * FROM users WHERE id = ?", {userId});
struct Param {
    enum Type { Null, Int, Real, Text } type = Null;
    int64_t i = 0;
    double d = 0;
    std::string s;
    Param(std::nullptr_t) {}
    Param(int v) : type(Int), i(v) {}
    Param(long v) : type(Int), i(v) {}
    Param(long long v) : type(Int), i(v) {}
    Param(unsigned long v) : type(Int), i((int64_t)v) {}
    Param(double v) : type(Real), d(v) {}
    Param(const std::string& v) : type(Text), s(v) {}
    Param(const char* v) : type(Text), s(v) {}
};

class DbError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// The only class that talks to SQLite. Thread-safe (one connection, one mutex).
class DatabaseManager {
public:
    explicit DatabaseManager(const std::string& path);
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    void initSchema(const std::string& schemaPath);
    void execute(const std::string& sql, const std::vector<Param>& params = {});
    int64_t insert(const std::string& sql, const std::vector<Param>& params = {});  // returns rowid
    nlohmann::json query(const std::string& sql, const std::vector<Param>& params = {});  // array of objects
    std::string sqliteVersion() const;

private:
    void run(const std::string& sql, const std::vector<Param>& params, nlohmann::json* rows, int64_t* lastId);
    sqlite3* db_ = nullptr;
    std::mutex mu_;
};
