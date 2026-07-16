#include <catch2/catch.hpp>
#include <filesystem>
#include <stdexcept>
#include <cstddef>
#include <cstdint>
#include <string>
// #include <functional>
#include <chrono>
// #include <optional>
#include "test_db_helper.hpp"
#include "rq_status_enum.hpp"
#include "sonora.hpp"

static const std::filesystem::path test_data_dir_path = std::filesystem::path(TEST_DATA_DIR);
static const std::filesystem::path db_dir_path = test_data_dir_path / "db";
static const std::filesystem::path full_samples_dir_path = test_data_dir_path / "samples" / "full_samples";

static const std::string& samples_db_path = std::string(db_dir_path / "sample_fingerprints.db");

static const std::string& dummy_db_base = "dummy_test_sonora.db";
static const std::string& dummy_db_path = std::string(db_dir_path / dummy_db_base);
static const std::string& dummy_db_wal_path = std::string(db_dir_path / (dummy_db_base + "-wal"));
static const std::string& dummy_db_shm_path = std::string(db_dir_path / (dummy_db_base + "-shm"));

static const std::filesystem::path samples_path = full_samples_dir_path / "sample_3s_16_khz.wav";

static const uint32_t downsmp_freq     = 8000   ;
static const std::size_t fir_coefs     = 51     ;
static const float frame_duration      = .01f   ;
static const uint32_t feature_ratio    = 10     ;
static const uint8_t window_size       = 3      ;
static const uint8_t peak_number       = 3      ;

const uint64_t max_index_rqs                    = UINT64_MAX                ;
const std::chrono::minutes index_expire_mins    = std::chrono::minutes(10)  ;
const uint64_t max_index_threads                = 1                         ;
const uint64_t max_match_rqs                    = UINT64_MAX                ;
const std::chrono::minutes match_expire_mins    = std::chrono::minutes(10)  ;
const uint64_t max_match_threads                = 16                        ;

TEST_CASE("Sonora: Constructor with custom parameters", "[Sonora][Constructor]")
{
    SECTION("Correct custom parameters")
    {
        SECTION("Existing database")
        {
            REQUIRE_NOTHROW(Sonora( downsmp_freq        ,
                                    samples_db_path     ,
                                    fir_coefs           ,
                                    frame_duration      ,
                                    feature_ratio       ,
                                    window_size         ,
                                    peak_number         ,
                                    max_index_rqs       ,
                                    index_expire_mins   ,
                                    max_index_threads   ,
                                    max_match_rqs       ,  
                                    match_expire_mins   ,
                                    max_match_threads   ));
            
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
            REQUIRE_NOTHROW(Sonora( downsmp_freq        ,
                                    dummy_db_path       ,
                                    fir_coefs           ,
                                    frame_duration      ,
                                    feature_ratio       ,
                                    window_size         ,
                                    peak_number         ,
                                    max_index_rqs       ,
                                    index_expire_mins   ,
                                    max_index_threads   ,
                                    max_match_rqs       ,  
                                    match_expire_mins   ,
                                    max_match_threads   ));

            REQUIRE(DBHelper::parametersMatch(  dummy_db_path   ,
                                                downsmp_freq    ,
                                                fir_coefs       ,
                                                frame_duration  ,
                                                feature_ratio   ,
                                                window_size     ,
                                                peak_number     ));

            std::filesystem::remove(dummy_db_path);
        }
    }

    SECTION("Invalid downsampling frequency")
    {
        REQUIRE_THROWS_AS(Sonora(   0                   ,
                                    samples_db_path     ,
                                    fir_coefs           ,
                                    frame_duration      ,
                                    feature_ratio       ,
                                    window_size         ,
                                    peak_number         ,
                                    max_index_rqs       ,
                                    index_expire_mins   ,
                                    max_index_threads   ,
                                    max_match_rqs       ,  
                                    match_expire_mins   ,
                                    max_match_threads   ),
                                    std::invalid_argument);        
    }

    SECTION("Database related")
    {
        SECTION("No path to database was provided")
        {
            REQUIRE_THROWS_AS(Sonora(   downsmp_freq        ,
                                        ""                  ,
                                        fir_coefs           ,
                                        frame_duration      ,
                                        feature_ratio       ,
                                        window_size         ,
                                        peak_number         ,
                                        max_index_rqs       ,
                                        index_expire_mins   ,
                                        max_index_threads   ,
                                        max_match_rqs       ,  
                                        match_expire_mins   ,
                                        max_match_threads   ),
                                        std::invalid_argument);
        }

        SECTION("Directory in provided path does not exist")
        {
            const std::string& non_existing_db_path = std::string(db_dir_path / "non_existing_dir" / "dummy.db");
        
            REQUIRE_THROWS_AS(Sonora(   downsmp_freq        ,
                                        non_existing_db_path,
                                        fir_coefs           ,
                                        frame_duration      ,
                                        feature_ratio       ,
                                        window_size         ,
                                        peak_number         ,
                                        max_index_rqs       ,
                                        index_expire_mins   ,
                                        max_index_threads   ,
                                        max_match_rqs       ,  
                                        match_expire_mins   ,
                                        max_match_threads   ),
                                        std::invalid_argument);
        }

        SECTION("Parent path to target database is not a directory")
        {
            const std::string& non_dir_db_path = std::string(full_samples_dir_path / "sample_3s_16_khz.wav" / "dummy.db");
        
            REQUIRE_THROWS_AS(Sonora(   downsmp_freq        ,
                                        non_dir_db_path     ,
                                        fir_coefs           ,
                                        frame_duration      ,
                                        feature_ratio       ,
                                        window_size         ,
                                        peak_number         ,
                                        max_index_rqs       ,
                                        index_expire_mins   ,
                                        max_index_threads   ,
                                        max_match_rqs       ,  
                                        match_expire_mins   ,
                                        max_match_threads   ),
                                        std::invalid_argument);
        }

        SECTION("Valid path to a file that is not a database")
        {
            const std::string& non_db_path = std::string(db_dir_path / "not_a_db.txt");
        
            REQUIRE_THROWS_AS(Sonora(   downsmp_freq        ,
                                        non_db_path         ,
                                        fir_coefs           ,
                                        frame_duration      ,
                                        feature_ratio       ,
                                        window_size         ,
                                        peak_number         ,
                                        max_index_rqs       ,
                                        index_expire_mins   ,
                                        max_index_threads   ,
                                        max_match_rqs       ,  
                                        match_expire_mins   ,
                                        max_match_threads   ),
                                        std::runtime_error  );
        }

        SECTION("Inconsistent input parameters (different from db\'s)")
        {
            SECTION("Invalid downsampling frequency")
            {
                REQUIRE_THROWS_AS(Sonora(   downsmp_freq + 1    ,
                                            samples_db_path     ,
                                            fir_coefs           ,
                                            frame_duration      ,
                                            feature_ratio       ,
                                            window_size         ,
                                            peak_number         ,
                                            max_index_rqs       ,
                                            index_expire_mins   ,
                                            max_index_threads   ,
                                            max_match_rqs       ,  
                                            match_expire_mins   ,
                                            max_match_threads   ),
                                            std::invalid_argument);
            }

            SECTION("Invalid number of FIR filter coeficients")
            {
                REQUIRE_THROWS_AS(Sonora(   downsmp_freq        ,
                                            samples_db_path     ,
                                            fir_coefs + 1       ,
                                            frame_duration      ,
                                            feature_ratio       ,
                                            window_size         ,
                                            peak_number         ,
                                            max_index_rqs       ,
                                            index_expire_mins   ,
                                            max_index_threads   ,
                                            max_match_rqs       ,  
                                            match_expire_mins   ,
                                            max_match_threads   ),
                                            std::invalid_argument);
            }

            SECTION("Invalid frame duration")
            {
                REQUIRE_THROWS_AS(Sonora(   downsmp_freq        ,
                                            samples_db_path     ,
                                            fir_coefs           ,
                                            frame_duration + .1f,
                                            feature_ratio       ,
                                            window_size         ,
                                            peak_number         ,
                                            max_index_rqs       ,
                                            index_expire_mins   ,
                                            max_index_threads   ,
                                            max_match_rqs       ,  
                                            match_expire_mins   ,
                                            max_match_threads   ),
                                            std::invalid_argument);
            }

            SECTION("Invalid feature ratio")
            {
                REQUIRE_THROWS_AS(Sonora(   downsmp_freq        ,
                                            samples_db_path     ,
                                            fir_coefs           ,
                                            frame_duration      ,
                                            feature_ratio + 1   ,
                                            window_size         ,
                                            peak_number         ,
                                            max_index_rqs       ,
                                            index_expire_mins   ,
                                            max_index_threads   ,
                                            max_match_rqs       ,  
                                            match_expire_mins   ,
                                            max_match_threads   ),
                                            std::invalid_argument);
            }

            SECTION("Invalid window size")
            {
                REQUIRE_THROWS_AS(Sonora(   downsmp_freq        ,
                                            samples_db_path     ,
                                            fir_coefs           ,
                                            frame_duration      ,
                                            feature_ratio       ,
                                            window_size + 1     ,
                                            peak_number         ,
                                            max_index_rqs       ,
                                            index_expire_mins   ,
                                            max_index_threads   ,
                                            max_match_rqs       ,  
                                            match_expire_mins   ,
                                            max_match_threads   ),
                                            std::invalid_argument);
            }

            SECTION("Invalid number of feature peaks")
            {
                REQUIRE_THROWS_AS(Sonora(   downsmp_freq        ,
                                            samples_db_path     ,
                                            fir_coefs           ,
                                            frame_duration      ,
                                            feature_ratio       ,
                                            window_size         ,
                                            peak_number + 1     ,
                                            max_index_rqs       ,
                                            index_expire_mins   ,
                                            max_index_threads   ,
                                            max_match_rqs       ,  
                                            match_expire_mins   ,
                                            max_match_threads   ),
                                            std::invalid_argument);
            }
        }
    }

    SECTION("Invalid parameters")
    {
        SECTION("Invalid downsampling frequency")
        {
            REQUIRE_THROWS_AS(Sonora(   0                   ,
                                        samples_db_path     ,
                                        fir_coefs           ,
                                        frame_duration      ,
                                        feature_ratio       ,
                                        window_size         ,
                                        peak_number         ,
                                        max_index_rqs       ,
                                        index_expire_mins   ,
                                        max_index_threads   ,
                                        max_match_rqs       ,  
                                        match_expire_mins   ,
                                        max_match_threads   ),
                                        std::invalid_argument);
        }

        SECTION("Invalid number of FIR filter coeficients")
        {
            REQUIRE_THROWS_AS(Sonora(   downsmp_freq        ,
                                        samples_db_path     ,
                                        0                   ,
                                        frame_duration      ,
                                        feature_ratio       ,
                                        window_size         ,
                                        peak_number         ,
                                        max_index_rqs       ,
                                        index_expire_mins   ,
                                        max_index_threads   ,
                                        max_match_rqs       ,  
                                        match_expire_mins   ,
                                        max_match_threads   ),
                                        std::invalid_argument);
        }

        SECTION("Invalid frame duration")
        {
            REQUIRE_THROWS_AS(Sonora(   downsmp_freq        ,
                                        samples_db_path     ,
                                        fir_coefs           ,
                                        .0f                 ,
                                        feature_ratio       ,
                                        window_size         ,
                                        peak_number         ,
                                        max_index_rqs       ,
                                        index_expire_mins   ,
                                        max_index_threads   ,
                                        max_match_rqs       ,  
                                        match_expire_mins   ,
                                        max_match_threads   ),
                                        std::invalid_argument);
        }

        SECTION("Invalid feature ratio")
        {
            REQUIRE_THROWS_AS(Sonora(   downsmp_freq        ,
                                        samples_db_path     ,
                                        fir_coefs           ,
                                        frame_duration      ,
                                        0                   ,
                                        window_size         ,
                                        peak_number         ,
                                        max_index_rqs       ,
                                        index_expire_mins   ,
                                        max_index_threads   ,
                                        max_match_rqs       ,  
                                        match_expire_mins   ,
                                        max_match_threads   ),
                                        std::invalid_argument);
        }

        SECTION("Invalid window size")
        {
            REQUIRE_THROWS_AS(Sonora(   downsmp_freq        ,
                                        samples_db_path     ,
                                        fir_coefs           ,
                                        frame_duration      ,
                                        feature_ratio       ,
                                        0                   ,
                                        peak_number         ,
                                        max_index_rqs       ,
                                        index_expire_mins   ,
                                        max_index_threads   ,
                                        max_match_rqs       ,  
                                        match_expire_mins   ,
                                        max_match_threads   ),
                                        std::invalid_argument);
        }

        SECTION("Invalid number of feature peaks")
        {
            REQUIRE_THROWS_AS(Sonora(   downsmp_freq        ,
                                        samples_db_path     ,
                                        fir_coefs           ,
                                        frame_duration      ,
                                        feature_ratio       ,
                                        window_size         ,
                                        0                   ,
                                        max_index_rqs       ,
                                        index_expire_mins   ,
                                        max_index_threads   ,
                                        max_match_rqs       ,  
                                        match_expire_mins   ,
                                        max_match_threads   ),
                                        std::invalid_argument);
        }

        SECTION("Invalid number of index op expire minutes")
        {
            REQUIRE_THROWS_AS(Sonora(   downsmp_freq            ,
                                        samples_db_path         ,
                                        fir_coefs               ,
                                        frame_duration          ,
                                        feature_ratio           ,
                                        window_size             ,
                                        peak_number             ,
                                        max_index_rqs           ,
                                        std::chrono::minutes{0} ,
                                        max_index_threads       ,
                                        max_match_rqs           ,
                                        match_expire_mins       ,
                                        max_match_threads       ),
                                        std::invalid_argument);
        }

        SECTION("Zero threads for non-null index ops queue")
        {
            REQUIRE_THROWS_AS(Sonora(   downsmp_freq        ,
                                        samples_db_path     ,
                                        fir_coefs           ,
                                        frame_duration      ,
                                        feature_ratio       ,
                                        window_size         ,
                                        peak_number         ,
                                        max_index_rqs       ,
                                        index_expire_mins   ,
                                        0                   ,
                                        max_match_rqs       ,  
                                        match_expire_mins   ,
                                        max_match_threads   ),
                                        std::invalid_argument);
        }

                SECTION("Invalid number of index op expire minutes")
        {
            REQUIRE_THROWS_AS(Sonora(   downsmp_freq            ,
                                        samples_db_path         ,
                                        fir_coefs               ,
                                        frame_duration          ,
                                        feature_ratio           ,
                                        window_size             ,
                                        peak_number             ,
                                        max_index_rqs           ,
                                        index_expire_mins       ,
                                        max_index_threads       ,
                                        max_match_rqs           ,
                                        std::chrono::minutes{0} ,
                                        max_match_threads       ),
                                        std::invalid_argument);
        }

        SECTION("Zero threads for non-null index ops queue")
        {
            REQUIRE_THROWS_AS(Sonora(   downsmp_freq        ,
                                        samples_db_path     ,
                                        fir_coefs           ,
                                        frame_duration      ,
                                        feature_ratio       ,
                                        window_size         ,
                                        peak_number         ,
                                        max_index_rqs       ,
                                        index_expire_mins   ,
                                        max_index_threads   ,
                                        max_match_rqs       ,  
                                        match_expire_mins   ,
                                        0                   ),
                                        std::invalid_argument);
        }
    }
}

TEST_CASE("Sonora: run", "[Sonora][run]")
{
    SECTION("Execute run method once")
    {
        Sonora sonora(  downsmp_freq        ,
                        samples_db_path     ,
                        fir_coefs           ,
                        frame_duration      ,
                        feature_ratio       ,
                        window_size         ,
                        peak_number         ,
                        max_index_rqs       ,
                        index_expire_mins   ,
                        max_index_threads   ,
                        max_match_rqs       ,  
                        match_expire_mins   ,
                        max_match_threads   );

        REQUIRE_NOTHROW(sonora.run());
        REQUIRE(sonora.isIndexerRunning());
        REQUIRE(sonora.isMatcherRunning());
    }

    SECTION("Execute run method twice")
    {
        Sonora sonora(  downsmp_freq        ,
                        samples_db_path     ,
                        fir_coefs           ,
                        frame_duration      ,
                        feature_ratio       ,
                        window_size         ,
                        peak_number         ,
                        max_index_rqs       ,
                        index_expire_mins   ,
                        max_index_threads   ,
                        max_match_rqs       ,  
                        match_expire_mins   ,
                        max_match_threads   );

        REQUIRE_NOTHROW(sonora.run());
        REQUIRE(sonora.isIndexerRunning());
        REQUIRE(sonora.isMatcherRunning());
        REQUIRE_NOTHROW(sonora.run());
        REQUIRE(sonora.isIndexerRunning());
        REQUIRE(sonora.isMatcherRunning());
    }
}

TEST_CASE("Sonora: end", "[Sonora][end]")
{
    SECTION("Execute end method once")
    {
        Sonora sonora(  downsmp_freq        ,
                        samples_db_path     ,
                        fir_coefs           ,
                        frame_duration      ,
                        feature_ratio       ,
                        window_size         ,
                        peak_number         ,
                        max_index_rqs       ,
                        index_expire_mins   ,
                        max_index_threads   ,
                        max_match_rqs       ,  
                        match_expire_mins   ,
                        max_match_threads   );

        sonora.run();
        REQUIRE_NOTHROW(sonora.end());
        REQUIRE_FALSE(sonora.isIndexerRunning());
        REQUIRE_FALSE(sonora.isMatcherRunning());
    }

    SECTION("Execute end method once")
    {
        Sonora sonora(  downsmp_freq        ,
                        samples_db_path     ,
                        fir_coefs           ,
                        frame_duration      ,
                        feature_ratio       ,
                        window_size         ,
                        peak_number         ,
                        max_index_rqs       ,
                        index_expire_mins   ,
                        max_index_threads   ,
                        max_match_rqs       ,  
                        match_expire_mins   ,
                        max_match_threads   );

        sonora.run();
        REQUIRE_NOTHROW(sonora.end());
        REQUIRE_FALSE(sonora.isIndexerRunning());
        REQUIRE_FALSE(sonora.isMatcherRunning());
        REQUIRE_NOTHROW(sonora.end());
        REQUIRE_FALSE(sonora.isIndexerRunning());
        REQUIRE_FALSE(sonora.isMatcherRunning());
    }

    SECTION("Execute end method with no preceeding run")
    {
        Sonora sonora(  downsmp_freq        ,
                        samples_db_path     ,
                        fir_coefs           ,
                        frame_duration      ,
                        feature_ratio       ,
                        window_size         ,
                        peak_number         ,
                        max_index_rqs       ,
                        index_expire_mins   ,
                        max_index_threads   ,
                        max_match_rqs       ,  
                        match_expire_mins   ,
                        max_match_threads   );
        
        REQUIRE_NOTHROW(sonora.end());
        REQUIRE_FALSE(sonora.isIndexerRunning());
        REQUIRE_FALSE(sonora.isMatcherRunning());
    }
}

TEST_CASE("Sonora: index", "[Sonora][index]")
{
    SECTION("Get proper job id")
    {
        Sonora sonora(  downsmp_freq        ,
                        samples_db_path     ,
                        fir_coefs           ,
                        frame_duration      ,
                        feature_ratio       ,
                        window_size         ,
                        peak_number         ,
                        max_index_rqs       ,
                        index_expire_mins   ,
                        max_index_threads   ,
                        max_match_rqs       ,  
                        match_expire_mins   ,
                        max_match_threads   );

        sonora.run();

        REQUIRE(sonora.index(std::string(samples_path)) == 0);
    }

    SECTION("Null path")
    {
        Sonora sonora(  downsmp_freq        ,
                        samples_db_path     ,
                        fir_coefs           ,
                        frame_duration      ,
                        feature_ratio       ,
                        window_size         ,
                        peak_number         ,
                        max_index_rqs       ,
                        index_expire_mins   ,
                        max_index_threads   ,
                        max_match_rqs       ,  
                        match_expire_mins   ,
                        max_match_threads   );
        sonora.run();     

        REQUIRE_THROWS_AS(sonora.index(""), std::invalid_argument);
    }

    SECTION("File errors")
    {
        Sonora sonora(  downsmp_freq        ,
                        samples_db_path     ,
                        fir_coefs           ,
                        frame_duration      ,
                        feature_ratio       ,
                        window_size         ,
                        peak_number         ,
                        max_index_rqs       ,
                        index_expire_mins   ,
                        max_index_threads   ,
                        max_match_rqs       ,  
                        match_expire_mins   ,
                        max_match_threads   );

        sonora.run();              

        SECTION("Non-existing path")
        {
            static const std::string non_existing_path = test_data_dir_path / "samples" / "full_samples" / "non_existing";

            REQUIRE_THROWS_AS(sonora.index(non_existing_path), std::invalid_argument);
        }

        SECTION("Path does not belong to a regular file")
        {
            REQUIRE_THROWS_AS(sonora.index(full_samples_dir_path), std::invalid_argument);
        }
    }

    SECTION("Push to full / null queue")
    {
        Sonora sonora(  downsmp_freq        ,
                        samples_db_path     ,
                        fir_coefs           ,
                        frame_duration      ,
                        feature_ratio       ,
                        window_size         ,
                        peak_number         ,
                        0                   ,
                        index_expire_mins   ,
                        0                   ,
                        max_match_rqs       ,  
                        match_expire_mins   ,
                        max_match_threads   );

        sonora.run();

        REQUIRE(sonora.index(std::string(samples_path)) == std::nullopt);
    }
}

TEST_CASE("Sonora: match", "[Sonora][match]")
{
    SECTION("Get proper job id")
    {
        Sonora sonora(  downsmp_freq        ,
                        samples_db_path     ,
                        fir_coefs           ,
                        frame_duration      ,
                        feature_ratio       ,
                        window_size         ,
                        peak_number         ,
                        max_index_rqs       ,
                        index_expire_mins   ,
                        max_index_threads   ,
                        max_match_rqs       ,  
                        match_expire_mins   ,
                        max_match_threads   );

        sonora.run();

        REQUIRE(sonora.match(std::string(samples_path)) == 0);
    }

    SECTION("Null path")
    {
        Sonora sonora(  downsmp_freq        ,
                        samples_db_path     ,
                        fir_coefs           ,
                        frame_duration      ,
                        feature_ratio       ,
                        window_size         ,
                        peak_number         ,
                        max_index_rqs       ,
                        index_expire_mins   ,
                        max_index_threads   ,
                        max_match_rqs       ,  
                        match_expire_mins   ,
                        max_match_threads   );
        sonora.run();     

        REQUIRE_THROWS_AS(sonora.match(""), std::invalid_argument);
    }

    SECTION("File errors")
    {
        Sonora sonora(  downsmp_freq        ,
                        samples_db_path     ,
                        fir_coefs           ,
                        frame_duration      ,
                        feature_ratio       ,
                        window_size         ,
                        peak_number         ,
                        max_index_rqs       ,
                        index_expire_mins   ,
                        max_index_threads   ,
                        max_match_rqs       ,  
                        match_expire_mins   ,
                        max_match_threads   );

        sonora.run();              

        SECTION("Non-existing path")
        {
            static const std::string non_existing_path = test_data_dir_path / "samples" / "full_samples" / "non_existing";

            REQUIRE_THROWS_AS(sonora.match(non_existing_path), std::invalid_argument);
        }

        SECTION("Path does not belong to a regular file")
        {
            REQUIRE_THROWS_AS(sonora.match(full_samples_dir_path), std::invalid_argument);
        }
    }

    SECTION("Push to full / null queue")
    {
        Sonora sonora(  downsmp_freq        ,
                        samples_db_path     ,
                        fir_coefs           ,
                        frame_duration      ,
                        feature_ratio       ,
                        window_size         ,
                        peak_number         ,
                        max_index_rqs       ,
                        index_expire_mins   ,
                        max_index_threads   ,
                        0                   ,  
                        match_expire_mins   ,
                        0                   );

        sonora.run();

        REQUIRE(sonora.match(std::string(samples_path)) == std::nullopt);
    }
}
