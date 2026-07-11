#include <catch2/catch.hpp>
#include <filesystem>
#include <stdexcept>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "audio_db_indexer.hpp"

std::filesystem::path db_dir_path = std::filesystem::path(TEST_DATA_DIR);
const std::string& dummy_db_path = std::string(db_dir_path / "dummy.db");

const uint32_t downsmp_freq     = 16000 ;
const std::size_t fir_coefs     = 101   ;
const float frame_duration      = .02f  ;
const uint32_t feature_ratio    = 5     ;
const uint8_t window_size       = 5     ;
const uint8_t peak_number       = 5     ;

#include <sqlite3.h>
#include <memory>
#include <stdexcept>
#include <string>

using sqlite_ptr_t = std::unique_ptr<sqlite3, decltype(&sqlite3_close)>;
using statement_ptr_t = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

bool parametersMatch(   const std::string& db_path  ,
                        const int downsmp_freq      ,
                        const int fir_coefs         ,
                        const float frame_duration  ,
                        const int feature_ratio     ,
                        const int window_size       ,
                        const int peak_number       )
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

bool songExists(const std::string& db_path, const int song_id, const std::string& song_name)
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

bool fingerprintExists(const std::string& db_path, const std::string& hash, const int song_id, const int frame_idx)
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

bool allFingerprintsExist(const std::string& db_path, int song_id, const std::unordered_map<std::size_t, std::vector<uint32_t>>& frame_hashes)
{
    for (const auto& [frame_idx, hashes] : frame_hashes)
        for (const auto& hash : hashes)
            if (!fingerprintExists(db_path, std::to_string(hash), song_id, static_cast<int>(frame_idx)))
                return false;

    return true;
}

TEST_CASE("Audio DB Indexer: Constructor with custom parameters", "[Audio DB Indexer][Constructor]")
{
    const std::string& samples_db_path = std::string(db_dir_path / "sample_fingerprints.db");

    SECTION("Existing database")
    {        
        REQUIRE_NOTHROW(AudioDBIndexer( samples_db_path ,
                                        downsmp_freq    ,
                                        fir_coefs       ,
                                        frame_duration  ,
                                        feature_ratio   ,
                                        window_size     ,
                                        peak_number     ));
        
        REQUIRE(parametersMatch(samples_db_path ,
                                downsmp_freq    ,
                                fir_coefs       ,
                                frame_duration  ,
                                feature_ratio   ,
                                window_size     ,
                                peak_number     ));
    }

    SECTION("No existing database path but valid directory")
    {
        REQUIRE_NOTHROW(AudioDBIndexer( dummy_db_path   ,
                                        downsmp_freq    ,
                                        fir_coefs       ,
                                        frame_duration  ,
                                        feature_ratio   ,
                                        window_size     ,
                                        peak_number     ));

        REQUIRE(parametersMatch(dummy_db_path   ,
                                downsmp_freq    ,
                                fir_coefs       ,
                                frame_duration  ,
                                feature_ratio   ,
                                window_size     ,
                                peak_number     ));

        std::filesystem::remove(dummy_db_path);
    }

    SECTION("Directory in provided path does not exist")
    {
        const std::string& non_existing_db_path = std::string(db_dir_path / "non_existing_dir" / "dummy.db");
    
        REQUIRE_THROWS_AS(AudioDBIndexer(   non_existing_db_path,
                                            downsmp_freq        ,
                                            fir_coefs           ,
                                            frame_duration      ,
                                            feature_ratio       ,
                                            window_size         ,
                                            peak_number         ),
                                            std::invalid_argument);
    }

    SECTION("Parent path to target database is not a directory")
    {
        const std::string& non_existing_db_path = std::string(db_dir_path / "sample_3s_16_khz.wav" / "dummy.db");
    
        REQUIRE_THROWS_AS(AudioDBIndexer(   non_existing_db_path,
                                            downsmp_freq        ,
                                            fir_coefs           ,
                                            frame_duration      ,
                                            feature_ratio       ,
                                            window_size         ,
                                            peak_number         ),
                                            std::invalid_argument);
    }

    SECTION("Valid path to a file that is not a database")
    {
        const std::string& non_existing_db_path = std::string(db_dir_path / "not_a_db.txt");

        REQUIRE_THROWS_AS(AudioDBIndexer(   non_existing_db_path,
                                            downsmp_freq        ,
                                            fir_coefs           ,
                                            frame_duration      ,
                                            feature_ratio       ,
                                            window_size         ,
                                            peak_number         ),
                                            std::runtime_error);
    }

    SECTION("Inconsistent input parameters (different from db\'s)")
    {
        SECTION("Invalid downsampling frequency")
        {
            REQUIRE_THROWS_AS(AudioDBIndexer(   samples_db_path ,
                                                downsmp_freq + 1,
                                                fir_coefs       ,
                                                frame_duration  ,
                                                feature_ratio   ,
                                                window_size     ,
                                                peak_number     ),
                                                std::invalid_argument);
        }

        SECTION("Invalid number of FIR filter coeficients")
        {
            REQUIRE_THROWS_AS(AudioDBIndexer(   samples_db_path ,
                                                downsmp_freq    ,
                                                fir_coefs + 1   ,
                                                frame_duration  ,
                                                feature_ratio   ,
                                                window_size     ,
                                                peak_number     ),
                                                std::invalid_argument);
        }

        SECTION("Invalid frame duration")
        {
            REQUIRE_THROWS_AS(AudioDBIndexer(   samples_db_path     ,
                                                downsmp_freq        ,
                                                fir_coefs           ,
                                                frame_duration + .1f,
                                                feature_ratio       ,
                                                window_size         ,
                                                peak_number         ),
                                                std::invalid_argument);
        }

        SECTION("Invalid feature ratio")
        {
            REQUIRE_THROWS_AS(AudioDBIndexer(   samples_db_path     ,
                                                downsmp_freq        ,
                                                fir_coefs           ,
                                                frame_duration      ,
                                                feature_ratio + 1   ,
                                                window_size         ,
                                                peak_number         ),
                                                std::invalid_argument);
        }
        
        SECTION("Invalid window size")
        {
            REQUIRE_THROWS_AS(AudioDBIndexer(   samples_db_path ,
                                                downsmp_freq    ,
                                                fir_coefs       ,
                                                frame_duration  ,
                                                feature_ratio   ,
                                                window_size + 1 ,
                                                peak_number     ),
                                                std::invalid_argument);
        }

        SECTION("Invalid number of feature peaks")
        {
            REQUIRE_THROWS_AS(AudioDBIndexer(   samples_db_path ,
                                                downsmp_freq    ,
                                                fir_coefs       ,
                                                frame_duration  ,
                                                feature_ratio   ,
                                                window_size     ,
                                                peak_number + 1 ),
                                                std::invalid_argument);
        }
    }
}

TEST_CASE("Audio DB Indexer: insertFingerprints", "[Audio DB Indexer][insertFingerprints]")
{
    AudioDBIndexer audio_db_indexer(dummy_db_path   ,
                                    downsmp_freq    ,
                                    fir_coefs       ,
                                    frame_duration  ,
                                    feature_ratio   ,
                                    window_size     ,
                                    peak_number     );

    const std::string& song_name = "dummy_song.wav";
    const std::unordered_map<std::size_t, std::vector<uint32_t>> frame_hashes =
    {
        {
            0,
            {1, 2, 3}
        },
        {
            1,
            {2, 3, 4}
        },
        {
            2,
            {3, 4, 5}
        },
        {
            3,
            {4, 5, 6}
        },
        {
            4,
            {5, 6, 7}
        },
    };

    SECTION("Method call with proper input parameters")
    {
        const int song_id = 1;

        SECTION("Add hashes for an unknown song")
        {
            REQUIRE_NOTHROW(audio_db_indexer.insertFingerprints(song_name, frame_hashes));
            REQUIRE(songExists(dummy_db_path, song_id, song_name));
            REQUIRE(allFingerprintsExist(dummy_db_path, song_id, frame_hashes));
        }

        SECTION("Overwrite hashes for an already known song")
        {
            REQUIRE_NOTHROW(audio_db_indexer.insertFingerprints(song_name, frame_hashes));
            REQUIRE(songExists(dummy_db_path, song_id, song_name));
            REQUIRE(allFingerprintsExist(dummy_db_path, song_id, frame_hashes));
        }
    }

    SECTION("Method call with null song name")
    {
        REQUIRE_THROWS_AS(audio_db_indexer.insertFingerprints("", frame_hashes), std::invalid_argument);
    }

    std::filesystem::remove(dummy_db_path);
}
