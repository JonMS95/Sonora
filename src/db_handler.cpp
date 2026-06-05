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

int DBHandler::_getOrCreateSongId(const std::string& song_name) const
{
    sqlite3_stmt* stmt = nullptr;
    int ret;

    const std::string& select_sql = "SELECT song_id FROM songs WHERE song_name = ?;";

    ret = sqlite3_prepare_v2(db_, select_sql.c_str(), -1, &stmt, nullptr);

    if(ret != SQLITE_OK)
        throw std::runtime_error("Failed to operate select " + song_name + " song");
    
    sqlite3_bind_text(stmt, 1, song_name.c_str(), -1, SQLITE_TRANSIENT);

    ret = sqlite3_step(stmt);

    if(ret == SQLITE_ROW)
    {
        int song_id = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        
        return song_id;
    }

    sqlite3_finalize(stmt);

    const std::string& insert_sql = "INSERT INTO songs(song_name) VALUES (?);";

    ret = sqlite3_prepare_v2(db_, insert_sql.c_str(), -1, &stmt, nullptr);

    if (ret != SQLITE_OK)
        throw std::runtime_error("Failed to prepare INSERT song");

    sqlite3_bind_text(stmt, 1, song_name.c_str(), -1, SQLITE_TRANSIENT);

    ret = sqlite3_step(stmt);

    if (ret != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        throw std::runtime_error("Failed to insert song");
    }

    sqlite3_finalize(stmt);

    return static_cast<int>(sqlite3_last_insert_rowid(db_));
}

// #include <iostream>

void DBHandler::insertFingerprints(const std::string& song_name, const std::unordered_map<int, std::vector<uint32_t>>& frame_hashes) const
{
    const int song_id = _getOrCreateSongId(song_name);

    const std::string& sql = "INSERT INTO fingerprints(hash, song_id, frame_idx) VALUES (?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error("Failed to prepare insert statement");

    // int cnt = 0;

    for (const auto& [frame_idx, hashes] : frame_hashes)
    {
        for (uint32_t hash : hashes)
        {
            sqlite3_bind_int(stmt, 1, static_cast<int>(hash));
            sqlite3_bind_int(stmt, 2, song_id);
            sqlite3_bind_int(stmt, 3, frame_idx);

            if (sqlite3_step(stmt) != SQLITE_DONE)
            {
                sqlite3_finalize(stmt);
                throw std::runtime_error("Failed to insert fingerprint");
            }

            sqlite3_reset(stmt);
        
            // std::cout << "Reg added: " << cnt++ << std::endl;
        }
    }

    sqlite3_finalize(stmt);
}
