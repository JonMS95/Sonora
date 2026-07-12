#include <catch2/catch.hpp>
#include <filesystem>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include "test_db_helper.hpp"
#include "audio_db_indexer.hpp"

static const std::filesystem::path test_data_dir_path = std::filesystem::path(TEST_DATA_DIR);
static const std::filesystem::path db_dir_path = test_data_dir_path / "db";
static const std::filesystem::path full_samples_dir_path = test_data_dir_path / "samples" / "full_samples";

static const std::string& dummy_db_path = std::string(db_dir_path / "dummy.db");
static const std::string& dummy_db_wal_path = std::string(db_dir_path / "dummy.db-wal");
static const std::string& dummy_db_shm_path = std::string(db_dir_path / "dummy.db-shm");

static const uint32_t downsmp_freq     = 8000   ;
static const std::size_t fir_coefs     = 51     ;
static const float frame_duration      = .01f   ;
static const uint32_t feature_ratio    = 10     ;
static const uint8_t window_size       = 3      ;
static const uint8_t peak_number       = 3      ;

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
        
        REQUIRE(DBHelper::parametersMatch(  samples_db_path ,
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

        REQUIRE(DBHelper::parametersMatch(  dummy_db_path   ,
                                            downsmp_freq    ,
                                            fir_coefs       ,
                                            frame_duration  ,
                                            feature_ratio   ,
                                            window_size     ,
                                            peak_number     ));

        std::filesystem::remove(dummy_db_path);
    }

    SECTION("No path to database was provided")
    {
        REQUIRE_THROWS_AS(AudioDBIndexer(   ""                  ,
                                            downsmp_freq        ,
                                            fir_coefs           ,
                                            frame_duration      ,
                                            feature_ratio       ,
                                            window_size         ,
                                            peak_number         ),
                                            std::invalid_argument);
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
        const std::string& non_existing_db_path = std::string(full_samples_dir_path / "sample_3s_16_khz.wav" / "dummy.db");
    
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
        const std::string& non_db_path = std::string(db_dir_path / "not_a_db.txt");

        REQUIRE_THROWS_AS(AudioDBIndexer(   non_db_path         ,
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
            REQUIRE(DBHelper::songExists(dummy_db_path, song_id, song_name));
            REQUIRE(DBHelper::allFingerprintsExist(dummy_db_path, song_id, frame_hashes));
        }

        SECTION("Overwrite hashes for an already known song")
        {
            REQUIRE_NOTHROW(audio_db_indexer.insertFingerprints(song_name, frame_hashes));
            REQUIRE(DBHelper::songExists(dummy_db_path, song_id, song_name));
            REQUIRE(DBHelper::allFingerprintsExist(dummy_db_path, song_id, frame_hashes));
        }
    }

    SECTION("Method call with null song name")
    {
        REQUIRE_THROWS_AS(audio_db_indexer.insertFingerprints("", frame_hashes), std::invalid_argument);
    }

    std::filesystem::remove(dummy_db_path);
    std::filesystem::remove(dummy_db_wal_path);
    std::filesystem::remove(dummy_db_shm_path);
}
