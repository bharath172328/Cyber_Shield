#include "database/DatabaseManager.h"
#include <fstream>
#include <sstream>

using nlohmann::json;

DatabaseManager::DatabaseManager(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        std::string msg = db_ ? sqlite3_errmsg(db_) : "out of memory";
        if (db_) sqlite3_close(db_);
        db_ = nullptr;
        throw DbError("Cannot open database: " + msg);
    }
    sqlite3_busy_timeout(db_, 5000);
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON; PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
}

DatabaseManager::~DatabaseManager() {
    if (db_) sqlite3_close(db_);
}

void DatabaseManager::initSchema(const std::string& schemaPath) {
    std::ifstream f(schemaPath);
    if (!f) throw DbError("Cannot read schema file: " + schemaPath);
    std::stringstream ss;
    ss << f.rdbuf();
    std::lock_guard<std::mutex> lk(mu_);
    char* err = nullptr;
    if (sqlite3_exec(db_, ss.str().c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "unknown";
        sqlite3_free(err);
        throw DbError("Schema error: " + msg);
    }
}

std::string DatabaseManager::sqliteVersion() const { return sqlite3_libversion(); }

void DatabaseManager::run(const std::string& sql, const std::vector<Param>& params, json* rows, int64_t* lastId) {
    std::lock_guard<std::mutex> lk(mu_);
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &st, nullptr) != SQLITE_OK)
        throw DbError(std::string("SQL prepare failed: ") + sqlite3_errmsg(db_));
    struct Guard { sqlite3_stmt* s; ~Guard() { sqlite3_finalize(s); } } guard{st};

    for (size_t n = 0; n < params.size(); ++n) {
        int idx = (int)n + 1;
        const Param& p = params[n];
        int rc = SQLITE_OK;
        switch (p.type) {
            case Param::Null: rc = sqlite3_bind_null(st, idx); break;
            case Param::Int: rc = sqlite3_bind_int64(st, idx, p.i); break;
            case Param::Real: rc = sqlite3_bind_double(st, idx, p.d); break;
            case Param::Text: rc = sqlite3_bind_text(st, idx, p.s.c_str(), (int)p.s.size(), SQLITE_TRANSIENT); break;
        }
        if (rc != SQLITE_OK) throw DbError(std::string("SQL bind failed: ") + sqlite3_errmsg(db_));
    }

    int rc;
    while ((rc = sqlite3_step(st)) == SQLITE_ROW) {
        if (!rows) continue;
        json row = json::object();
        int cols = sqlite3_column_count(st);
        for (int c = 0; c < cols; ++c) {
            const char* name = sqlite3_column_name(st, c);
            switch (sqlite3_column_type(st, c)) {
                case SQLITE_INTEGER: row[name] = (int64_t)sqlite3_column_int64(st, c); break;
                case SQLITE_FLOAT: row[name] = sqlite3_column_double(st, c); break;
                case SQLITE_TEXT: row[name] = reinterpret_cast<const char*>(sqlite3_column_text(st, c)); break;
                default: row[name] = nullptr;
            }
        }
        rows->push_back(std::move(row));
    }
    if (rc != SQLITE_DONE) throw DbError(std::string("SQL step failed: ") + sqlite3_errmsg(db_));
    if (lastId) *lastId = (int64_t)sqlite3_last_insert_rowid(db_);
}

void DatabaseManager::execute(const std::string& sql, const std::vector<Param>& params) {
    run(sql, params, nullptr, nullptr);
}

int64_t DatabaseManager::insert(const std::string& sql, const std::vector<Param>& params) {
    int64_t id = 0;
    run(sql, params, nullptr, &id);
    return id;
}

json DatabaseManager::query(const std::string& sql, const std::vector<Param>& params) {
    json rows = json::array();
    run(sql, params, &rows, nullptr);
    return rows;
}
