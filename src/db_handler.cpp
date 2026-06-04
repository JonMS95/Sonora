#include <string>
#include <filesystem>
#include <sqlite3.h>
#include "db_handler.hpp"

DBHandler::DBHandler(const std::string& db_path): db_(nullptr)
{
    // Check whether the path to the target database exists.
    std::filesystem::path fs_db_path(db_path);

    if(fs_db_path.has_parent_path())
    {
        if(!std::filesystem::exists(fs_db_path.parent_path()))
            throw std::runtime_error("Database directory does not exist: " + fs_db_path.parent_path().string());
        
        if(!std::filesystem::is_directory(fs_db_path.parent_path()))
            throw std::runtime_error("Path to database is not a directory");
    }

    int ret = sqlite3_open(db_path.c_str(), &db_);

    if(ret != SQLITE_OK)
    {
        const std::string err_str = sqlite3_errmsg(db_);
        sqlite3_close(db_);

        throw std::runtime_error("SQLite open failed: "+ err_str);
    }

    _createSongsTable();
    _createFingerprintsTable();
}

void DBHandler::_createSongsTable(void) const
{
    const char* table_sql =
        "CREATE TABLE IF NOT EXISTS songs"
        "("
            "song_id INTEGER PRIMARY KEY,"
            "song_name TEXT NOT NULL UNIQUE" // Automatically creates index.
        ");";

    char* err = nullptr;

    if (sqlite3_exec(db_, table_sql, nullptr, nullptr, &err) != SQLITE_OK)
    {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw std::runtime_error("songs table creation failed: " + msg);
    }
}

void DBHandler::_createFingerprintsTable(void) const
{
    const char* table_sql =
        "CREATE TABLE IF NOT EXISTS fingerprints"
        "("
            "hash INTEGER NOT NULL,"
            "song_id INTEGER NOT NULL,"
            "frame_idx INTEGER NOT NULL"
        ");";

    const char* index_sql =
        "CREATE INDEX IF NOT EXISTS idx_hash ON fingerprints(hash);";

    char* err = nullptr;

    if(sqlite3_exec(db_, table_sql, nullptr, nullptr, &err) != SQLITE_OK)
    {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw std::runtime_error("fingerprints table creation failed: " + msg);
    }

    if(sqlite3_exec(db_, index_sql, nullptr, nullptr, &err) != SQLITE_OK)
    {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw std::runtime_error("fingerprints index creation failed: " + msg);
    }
}

