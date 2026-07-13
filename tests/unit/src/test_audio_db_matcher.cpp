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

static const std::string& dummy_db_base = "dummy_test_audio_db_matcher.db";;
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
        REQUIRE_THROWS_AS(AudioDBMatcher(   dummy_db_path   ,
                                            downsmp_freq    ,
                                            fir_coefs       ,
                                            frame_duration  ,
                                            feature_ratio   ,
                                            window_size     ,
                                            peak_number     ),
                                            std::invalid_argument);
        
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

static std::filesystem::path removePartSuffix(const std::filesystem::path& p)
{
    std::string stem = p.stem().string();   // "sample_3s_16_khz_part_000"
    constexpr std::string_view marker = "_part_";

    auto pos = stem.rfind(marker);
    if (pos != std::string::npos)
        stem.erase(pos);

    return p.parent_path() / (stem + p.extension().string());
}

static bool checkAllPartFingerprints(   const std::filesystem::path sample_parts_dir_path   ,
                                        const std::string& db_path                          ,
                                        const uint32_t downsmp_freq                         ,
                                        const std::size_t fir_coefs                         ,
                                        const float frame_duration                          ,
                                        const uint32_t feature_ratio                        ,
                                        const uint8_t window_size                           ,
                                        const uint8_t peak_number                           )
{
    AudioDBMatcher audio_db_matcher(db_path         ,
                                    downsmp_freq    ,
                                    fir_coefs       ,
                                    frame_duration  ,
                                    feature_ratio   ,
                                    window_size     ,
                                    peak_number     );

    std::unordered_map<uint32_t, Preprocessor> preprocessor_map;
    SpectralAnalyzer spectral_analyzer(frame_duration, downsmp_freq, feature_ratio, peak_number);
    FingerprintGenerator fingerprint_generator(window_size);
    std::filesystem::path input;
    std::filesystem::path expected;
    uint32_t sample_rate;
    std::vector<float> prep_signal;
    std::vector<std::vector<std::size_t>> features;
    std::unordered_map<std::size_t, std::vector<uint32_t>> song_fingerprint;
    std::filesystem::path matcher_output_file;

    for (const auto& entry : std::filesystem::directory_iterator(sample_parts_dir_path))
    {
        if (!entry.is_regular_file())
            continue;

        input = entry.path();
        expected = removePartSuffix(input);

        sample_rate = Preprocessor::getFileSampleRate(input);
        preprocessor_map.try_emplace(sample_rate, sample_rate, downsmp_freq, fir_coefs);

        prep_signal = preprocessor_map.at(sample_rate).preprocessData(input);
        features = spectral_analyzer.analyze(prep_signal);
        song_fingerprint = fingerprint_generator.genFP(features);
        matcher_output_file = audio_db_matcher.queryHashes(song_fingerprint);

        if(expected.filename() != matcher_output_file.filename())
            return false;
    }

    return true;
}

TEST_CASE("Audio DB Matcher: queryHashes", "[Audio DB Matcher][queryHashes]")
{
    const std::filesystem::path sample_parts_dir_path = test_data_dir_path / "samples" / "sample_parts";

    REQUIRE(checkAllPartFingerprints(   sample_parts_dir_path   ,
                                        samples_db_path         ,
                                        downsmp_freq            ,
                                        fir_coefs               ,
                                        frame_duration          ,
                                        feature_ratio           ,
                                        window_size             ,
                                        peak_number             ));
}
