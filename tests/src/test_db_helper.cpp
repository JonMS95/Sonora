#include <memory>
#include <sqlite3.h>
#include <stdexcept>
#include "test_db_helper.hpp"

namespace DBHelper
{

using sqlite_ptr_t = std::unique_ptr<sqlite3, decltype(&sqlite3_close)>;
using statement_ptr_t = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

bool parametersMatch(   const std::string& db_path      ,
                        const uint32_t downsmp_freq     ,
                        const std::size_t fir_coefs     ,
                        const float frame_duration      ,
                        const uint32_t feature_ratio    ,
                        const uint8_t window_size       ,
                        const uint8_t peak_number       )
{
    sqlite3* raw_db = nullptr;

    int ret = sqlite3_open(db_path.c_str(), &raw_db);

    if (ret != SQLITE_OK)
    {
        std::string err = raw_db ? sqlite3_errmsg(raw_db)
                                 : "Unknown SQLite error";

        if (raw_db)
            sqlite3_close(raw_db);

        throw std::runtime_error("SQLite open failed: " + err);
    }

    sqlite_ptr_t db(raw_db, sqlite3_close);

    const char* sql =
        "SELECT EXISTS("
        "SELECT 1 FROM parameters "
        "WHERE downsmp_freq = ? "
        "AND fir_coefs = ? "
        "AND frame_duration = ? "
        "AND feature_ratio = ? "
        "AND window_size = ? "
        "AND peak_number = ?"
        ");";

    sqlite3_stmt* raw_stmt = nullptr;

    ret = sqlite3_prepare_v2(db.get(), sql, -1, &raw_stmt, nullptr);

    if (ret != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db.get()));

    statement_ptr_t stmt(raw_stmt, sqlite3_finalize);

    sqlite3_bind_int(stmt.get()     , 1, downsmp_freq   );
    sqlite3_bind_int(stmt.get()     , 2, fir_coefs      );
    sqlite3_bind_double(stmt.get()  , 3, frame_duration );
    sqlite3_bind_int(stmt.get()     , 4, feature_ratio  );
    sqlite3_bind_int(stmt.get()     , 5, window_size    );
    sqlite3_bind_int(stmt.get()     , 6, peak_number    );

    ret = sqlite3_step(stmt.get());

    if (ret != SQLITE_ROW)
        throw std::runtime_error("Failed to execute parameters lookup.");

    return sqlite3_column_int(stmt.get(), 0) != 0;
}

bool songExists(const std::string& db_path, const uint32_t song_id, const std::string& song_name)
{
    sqlite3* raw_db = nullptr;

    int ret = sqlite3_open(db_path.c_str(), &raw_db);

    if (ret != SQLITE_OK)
    {
        std::string err = raw_db ? sqlite3_errmsg(raw_db)
                                 : "Unknown SQLite error";

        if (raw_db)
            sqlite3_close(raw_db);

        throw std::runtime_error("SQLite open failed: " + err);
    }

    sqlite_ptr_t db(raw_db, sqlite3_close);

    const char* sql =
        "SELECT EXISTS("
        "SELECT 1 FROM songs "
        "WHERE song_id = ? AND song_name = ?"
        ");";

    sqlite3_stmt* raw_stmt = nullptr;

    ret = sqlite3_prepare_v2(db.get(), sql, -1, &raw_stmt, nullptr);

    if (ret != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db.get()));

    statement_ptr_t stmt(raw_stmt, sqlite3_finalize);

    sqlite3_bind_int(stmt.get(), 1, song_id);
    sqlite3_bind_text(stmt.get(), 2, song_name.c_str(), -1, SQLITE_TRANSIENT);

    ret = sqlite3_step(stmt.get());

    if (ret != SQLITE_ROW)
        throw std::runtime_error("Failed to execute song lookup query.");

    return sqlite3_column_int(stmt.get(), 0) != 0;
}

bool fingerprintExists(const std::string& db_path, const std::string& hash, const uint32_t song_id, const std::size_t frame_idx)
{
    sqlite3* raw_db = nullptr;

    int ret = sqlite3_open(db_path.c_str(), &raw_db);

    if (ret != SQLITE_OK)
    {
        std::string err = raw_db ? sqlite3_errmsg(raw_db)
                                 : "Unknown SQLite error";

        if (raw_db)
            sqlite3_close(raw_db);

        throw std::runtime_error("SQLite open failed: " + err);
    }

    sqlite_ptr_t db(raw_db, sqlite3_close);

    const char* sql =
        "SELECT EXISTS("
        "SELECT 1 FROM fingerprints "
        "WHERE hash = ? "
        "AND song_id = ? "
        "AND frame_idx = ?"
        ");";

    sqlite3_stmt* raw_stmt = nullptr;

    ret = sqlite3_prepare_v2(db.get(), sql, -1, &raw_stmt, nullptr);

    if (ret != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db.get()));

    statement_ptr_t stmt(raw_stmt, sqlite3_finalize);

    sqlite3_bind_text(stmt.get(), 1, hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 2, song_id);
    sqlite3_bind_int(stmt.get(), 3, frame_idx);

    ret = sqlite3_step(stmt.get());

    if (ret != SQLITE_ROW)
        throw std::runtime_error("Failed to execute fingerprint lookup.");

    return sqlite3_column_int(stmt.get(), 0) != 0;
}

bool allFingerprintsExist(const std::string& db_path, uint32_t song_id, const std::unordered_map<std::size_t, std::vector<uint32_t>>& frame_hashes)
{
    for (const auto& [frame_idx, hashes] : frame_hashes)
        for (const auto& hash : hashes)
            if (!fingerprintExists(db_path, std::to_string(hash), song_id, static_cast<int>(frame_idx)))
                return false;

    return true;
}

}
