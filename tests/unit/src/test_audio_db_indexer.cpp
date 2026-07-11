#include <catch2/catch.hpp>
#include <filesystem>
#include <stdexcept>
#include "audio_db_indexer.hpp"

TEST_CASE("Audio DB Indexer: Constructor with custom parameters", "[Audio DB Indexer][Constructor]")
{
    std::filesystem::path db_dir_path = std::filesystem::path(TEST_DATA_DIR);
    
    const uint32_t downsmp_freq     = 16000 ;
    const std::size_t fir_coefs     = 101   ;
    const float frame_duration      = .02f  ;
    const uint32_t feature_ratio    = 5     ;
    const uint8_t window_size       = 5     ;
    const uint8_t peak_number       = 5     ;

    SECTION("Existing database")
    {
        const std::string& samples_db_path = std::string(db_dir_path /= "sample_fingerprints.db");
        
        REQUIRE_NOTHROW(AudioDBIndexer( samples_db_path ,
                                        downsmp_freq    ,
                                        fir_coefs       ,
                                        frame_duration  ,
                                        feature_ratio   ,
                                        window_size     ,
                                        peak_number     ));
    }

    SECTION("No existing database path but valid directory")
    {
        const std::string& dummy_db_path = std::string(db_dir_path /= "dummy.db");

        REQUIRE_NOTHROW(AudioDBIndexer( dummy_db_path   ,
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

    // SECTION("")
}

// explicit AudioDBIndexer(const std::string& db_path  ,
//                         const uint32_t downsmp_freq ,
//                         const std::size_t fir_coefs ,
//                         const float frame_duration  ,
//                         const uint32_t feature_ratio,
//                         const uint8_t window_size   ,
//                         const uint8_t peak_number   );