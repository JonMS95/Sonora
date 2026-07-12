#include <catch2/catch.hpp>
#include <filesystem>
#include <stdexcept>
#include <cstddef>
#include <cstdint>
#include <string>
// #include <vector>
// #include <memory>
// #include <unordered_map>
#include <sqlite3.h>
#include "test_db_helper.hpp"
#include "audio_db_matcher.hpp"

static const std::filesystem::path db_dir_path = std::filesystem::path(TEST_DATA_DIR);
static const std::string& dummy_db_path = std::string(db_dir_path / "dummy.db");
// static const std::string& dummy_db_wal_path = std::string(db_dir_path / "dummy.db-wal");
// static const std::string& dummy_db_shm_path = std::string(db_dir_path / "dummy.db-shm");

static const uint32_t downsmp_freq     = 16000 ;
static const std::size_t fir_coefs     = 101   ;
static const float frame_duration      = .02f  ;
static const uint32_t feature_ratio    = 5     ;
static const uint8_t window_size       = 5     ;
static const uint8_t peak_number       = 5     ;

TEST_CASE("Audio DB Matcher: Constructor with custom parameters", "[Audio DB Matcher][Constructor]")
{
    const std::string& samples_db_path = std::string(db_dir_path / "sample_fingerprints.db");

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

    SECTION("No existing database path but valid directory")
    {
        REQUIRE_THROWS_AS(AudioDBMatcher(   dummy_db_path   ,
                                            downsmp_freq    ,
                                            fir_coefs       ,
                                            frame_duration  ,
                                            feature_ratio   ,
                                            window_size     ,
                                            peak_number     ),
                                            std::runtime_error);
        
        std::filesystem::remove(dummy_db_path);
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

// TEST_CASE("Audio DB Matcher: Constructor with custom parameters", "[Audio DB Matcher][Constructor]")
