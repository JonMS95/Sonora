#include <string>
#include <cstddef>
#include <filesystem>
#include <sqlite3.h>
#include "audio_db_base.hpp"
#include "audio_db_indexer.hpp"

AudioDBIndexer::AudioDBIndexer( const std::string& db_path  ,
                                const uint32_t downsmp_freq ,
                                const std::size_t fir_coefs ,
                                const float frame_duration  ,
                                const uint32_t feature_ratio,
                                const uint8_t window_size   ,
                                const uint8_t peak_number   ):
    AudioDBBase(db_path)
{
    _createParametersTable( downsmp_freq    ,
                            fir_coefs       ,
                            frame_duration  ,
                            feature_ratio   ,
                            window_size     ,
                            peak_number     );
    _createSongsTable();
    _createFingerprintsTable();
}

void AudioDBIndexer::_createParametersTable(const uint32_t downsmp_freq ,
                                            const std::size_t fir_coefs ,
                                            const float frame_duration  ,
                                            const uint32_t feature_ratio,
                                            const uint8_t window_size   ,
                                            const uint8_t peak_number   ) const
{
    const std::string& table_sql =
        "CREATE TABLE IF NOT EXISTS parameters"
        "("
            "downsmp_freq INTEGER,"
            "fir_coefs INTEGER,"
            "frame_duration REAL,"
            "feature_ratio INTEGER,"
            "window_size INTEGER,"
            "peak_number INTEGER"
        ");";

    char* err = nullptr;

    if(sqlite3_exec(db_, table_sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK)
    {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw std::runtime_error("params table creation failed: " + msg);
    }

    const std::string& insert_sql = "INSERT INTO parameters(downsmp_freq, fir_coefs, frame_duration, feature_ratio, window_size, peak_number) VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* raw_insert_stmt = nullptr;

    if(sqlite3_prepare_v2(db_, insert_sql.c_str(), -1, &raw_insert_stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error("Failed to prepare insert statement");
    
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> p_insert_stmt(raw_insert_stmt, &sqlite3_finalize);

    sqlite3_bind_int(   p_insert_stmt.get(), 1, downsmp_freq                        );
    sqlite3_bind_int(   p_insert_stmt.get(), 2, fir_coefs                           );
    sqlite3_bind_double(p_insert_stmt.get(), 3, static_cast<double>(frame_duration) );
    sqlite3_bind_int(   p_insert_stmt.get(), 4, feature_ratio                       );
    sqlite3_bind_int(   p_insert_stmt.get(), 5, window_size                         );
    sqlite3_bind_int(   p_insert_stmt.get(), 6, peak_number                         );

    int rc = sqlite3_step(p_insert_stmt.get());
    if(rc != SQLITE_DONE)
        throw std::runtime_error("Failed insert song");
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

    if(sqlite3_exec(db_, table_sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK)
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
    if(rc != SQLITE_DONE)
        throw std::runtime_error("Failed insert song");

    return static_cast<uint32_t>(sqlite3_last_insert_rowid(db_));
}

void AudioDBIndexer::insertFingerprints(const std::string& song_name, const std::unordered_map<std::size_t, std::vector<uint32_t>>& frame_hashes) const
{
    const uint32_t song_id = _getOrCreateSongId(song_name);

    const std::string& sql = "INSERT INTO fingerprints(hash, song_id, frame_idx) VALUES (?, ?, ?);";

    sqlite3_stmt* raw_stmt = nullptr;

    if(sqlite3_prepare_v2(db_, sql.c_str(), -1, &raw_stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error("Failed to prepare insert statement");
    
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> p_stmt(raw_stmt, &sqlite3_finalize);

    for(const auto& [frame_idx, hashes] : frame_hashes)
    {
        for(uint32_t hash : hashes)
        {
            sqlite3_bind_int(p_stmt.get(), 1, hash);
            sqlite3_bind_int(p_stmt.get(), 2, song_id);
            sqlite3_bind_int(p_stmt.get(), 3, frame_idx);

            if(sqlite3_step(p_stmt.get()) != SQLITE_DONE)
            {
                throw std::runtime_error("Failed to insert fingerprint");
            }

            sqlite3_reset(p_stmt.get());
            sqlite3_clear_bindings(p_stmt.get());
        }
    }
}
