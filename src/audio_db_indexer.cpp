#include <string>
#include <cstddef>
#include <filesystem>
#include <sqlite3.h>
#include "audio_db_base.hpp"
#include "audio_db_indexer.hpp"

AudioDBIndexer::AudioDBIndexer(const std::string& db_path):
    AudioDBBase(db_path)
{
    _createSongsTable();
    _createFingerprintsTable();
}

void AudioDBIndexer::_createSongsTable(void) const
{
    const std::string& table_sql =
        "CREATE TABLE IF NOT EXISTS songs"
        "("
            "song_id INTEGER PRIMARY KEY,"
            "song_name TEXT NOT NULL UNIQUE" // Automatically creates index.
        ");";

    char* err = nullptr;

    if (sqlite3_exec(db_, table_sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK)
    {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw std::runtime_error("songs table creation failed: " + msg);
    }
}

void AudioDBIndexer::_createFingerprintsTable(void) const
{
    const std::string& table_sql =
        "CREATE TABLE IF NOT EXISTS fingerprints"
        "("
            "hash INTEGER NOT NULL,"
            "song_id INTEGER NOT NULL,"
            "frame_idx INTEGER NOT NULL"
        ");";

    const std::string& index_sql =
        "CREATE INDEX IF NOT EXISTS idx_hash ON fingerprints(hash);";

    char* err = nullptr;

    if(sqlite3_exec(db_, table_sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK)
    {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw std::runtime_error("fingerprints table creation failed: " + msg);
    }

    if(sqlite3_exec(db_, index_sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK)
    {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw std::runtime_error("fingerprints index creation failed: " + msg);
    }
}

uint32_t AudioDBIndexer::_getOrCreateSongId(const std::string& song_name) const
{
    const std::string& select_sql = "SELECT song_id FROM songs WHERE song_name = ?;";

    sqlite3_stmt* raw_stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_, select_sql.c_str(), -1, &raw_stmt, nullptr);
    if (rc != SQLITE_OK)
        throw std::runtime_error("Failed SELECT");

    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>
        stmt(raw_stmt, &sqlite3_finalize);

    sqlite3_bind_text(stmt.get(), 1, song_name.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW)
        return static_cast<uint32_t>(sqlite3_column_int(stmt.get(), 0));

    const std::string insert_sql = "INSERT INTO songs(song_name) VALUES (?);";

    raw_stmt = nullptr;

    rc = sqlite3_prepare_v2(db_, insert_sql.c_str(), -1, &raw_stmt, nullptr);
    if (rc != SQLITE_OK)
        throw std::runtime_error("Failed INSERT");

    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> insert_stmt(raw_stmt, &sqlite3_finalize);

    sqlite3_bind_text(insert_stmt.get(), 1, song_name.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(insert_stmt.get());
    if (rc != SQLITE_DONE)
        throw std::runtime_error("Failed insert song");

    return static_cast<uint32_t>(sqlite3_last_insert_rowid(db_));
}

void AudioDBIndexer::insertFingerprints(const std::string& song_name, const std::unordered_map<std::size_t, std::vector<uint32_t>>& frame_hashes) const
{
    const uint32_t song_id = _getOrCreateSongId(song_name);

    const std::string& sql = "INSERT INTO fingerprints(hash, song_id, frame_idx) VALUES (?, ?, ?);";

    sqlite3_stmt* raw_stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &raw_stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error("Failed to prepare insert statement");
    
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> p_stmt(raw_stmt, &sqlite3_finalize);

    for (const auto& [frame_idx, hashes] : frame_hashes)
    {
        for (uint32_t hash : hashes)
        {
            sqlite3_bind_int(p_stmt.get(), 1, hash);
            sqlite3_bind_int(p_stmt.get(), 2, song_id);
            sqlite3_bind_int(p_stmt.get(), 3, frame_idx);

            if (sqlite3_step(p_stmt.get()) != SQLITE_DONE)
            {
                throw std::runtime_error("Failed to insert fingerprint");
            }

            sqlite3_reset(p_stmt.get());
        }
    }
}
