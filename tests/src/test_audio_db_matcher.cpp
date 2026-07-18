#include <catch2/catch.hpp>
#include <filesystem>
#include <stdexcept>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <sqlite3.h>
#include "test_db_helper.hpp"
#include "preprocessor.hpp"
#include "spectral_analyzer.hpp"
#include "fingerprint_generator.hpp"
#include "audio_db_matcher.hpp"

static const std::filesystem::path test_data_dir_path = std::filesystem::path(TEST_DATA_DIR);
static const std::filesystem::path db_dir_path = test_data_dir_path / "db";
static const std::filesystem::path full_samples_dir_path = test_data_dir_path / "samples" / "full_samples";
static const std::filesystem::path sample_parts_dir_path = test_data_dir_path / "samples" / "sample_parts";

static const std::string& samples_db_path = std::string(db_dir_path / "sample_fingerprints.db");
static const std::string& no_db_path = std::string(db_dir_path / "non_existing.db");

static const std::string& dummy_db_base = "dummy_test_audio_db_matcher.db";
static const std::string& dummy_db_path = std::string(db_dir_path / dummy_db_base);
static const std::string& dummy_db_wal_path = std::string(db_dir_path / (dummy_db_base + "-wal"));
static const std::string& dummy_db_shm_path = std::string(db_dir_path / (dummy_db_base + "-shm"));

static const uint32_t downsmp_freq     = 8000   ;
static const std::size_t fir_coefs     = 51     ;
static const float frame_duration      = .01f   ;
static const uint32_t feature_ratio    = 10     ;
static const uint8_t window_size       = 3      ;
static const uint8_t peak_number       = 3      ;

TEST_CASE("Audio DB Matcher: Constructor with custom parameters", "[Audio DB Matcher][Constructor]")
{
    SECTION("Existing database")
    {        
        REQUIRE_NOTHROW(AudioDBMatcher( samples_db_path ,
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

    SECTION("Valid directory but not a database path")
    {
        REQUIRE_THROWS_AS(AudioDBMatcher(   no_db_path      ,
                                            downsmp_freq    ,
                                            fir_coefs       ,
                                            frame_duration  ,
                                            feature_ratio   ,
                                            window_size     ,
                                            peak_number     ),
                                            std::invalid_argument);        
    }

    SECTION("Directory in provided path does not exist")
    {
        const std::string& non_existing_db_path = std::string(db_dir_path / "non_existing_dir" / "dummy.db");
    
        REQUIRE_THROWS_AS(AudioDBMatcher(   non_existing_db_path,
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
    
        REQUIRE_THROWS_AS(AudioDBMatcher(   non_existing_db_path,
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

        REQUIRE_THROWS_AS(AudioDBMatcher(   non_existing_db_path,
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
            REQUIRE_THROWS_AS(AudioDBMatcher(   samples_db_path ,
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
            REQUIRE_THROWS_AS(AudioDBMatcher(   samples_db_path ,
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
            REQUIRE_THROWS_AS(AudioDBMatcher(   samples_db_path     ,
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
            REQUIRE_THROWS_AS(AudioDBMatcher(   samples_db_path     ,
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
            REQUIRE_THROWS_AS(AudioDBMatcher(   samples_db_path ,
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
            REQUIRE_THROWS_AS(AudioDBMatcher(   samples_db_path ,
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

TEST_CASE("Audio DB Matcher: queryHashes", "[Audio DB Matcher][queryHashes]")
{
    AudioDBMatcher audio_db_matcher(dummy_db_path   ,
                                    downsmp_freq    ,
                                    fir_coefs       ,
                                    frame_duration  ,
                                    feature_ratio   ,
                                    window_size     ,
                                    peak_number     );

    const std::unordered_map<std::size_t, std::vector<uint32_t>> frame_hashes =
    {
        {0  ,   {6292737}},
        {1  ,   {5244161}},
        {2  ,   {0123456}},
        {3  ,   {5244161}},
        {4  ,   {5243905}},
        {5  ,   {9876543}},
        {6  ,   {4195585}},
        {7  ,   {4195329}},
        {8  ,   {5244161}},
        {9  ,   {4195329}},
        {10 ,   {5243905}},
    };

    REQUIRE(audio_db_matcher.queryHashes(frame_hashes) == "dummy_song_2");
}
