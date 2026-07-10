#include <catch2/catch.hpp>
#include "spectral_analyzer.hpp"

TEST_CASE("Spectral Analyzer: Constructor with custom parameters", "[Spectral Analyzer][Constructor]")
{
    const float frame_duration          = .2f   ;
    const uint32_t sampling_frequency   = 8000  ;
    const uint32_t feature_ratio        = 5     ;
    const uint8_t peak_number           = 4     ;

    SECTION("Valid custom parameters")
    {
        REQUIRE_NOTHROW(SpectralAnalyzer(frame_duration, sampling_frequency, feature_ratio, peak_number));
    }

    SECTION("Invalid frame length")
    {
        SECTION("Frame length < 0")
        {
            REQUIRE_THROWS_AS(SpectralAnalyzer(-frame_duration, sampling_frequency, feature_ratio, peak_number), std::invalid_argument);
        }

        SECTION("Frame length == 0")
        {
            REQUIRE_THROWS_AS(SpectralAnalyzer(0.0f, sampling_frequency, feature_ratio, peak_number), std::invalid_argument);
        }
    }

    SECTION("Invalid sampling frequency")
    {
        REQUIRE_THROWS_AS(SpectralAnalyzer(frame_duration, 0, feature_ratio, peak_number), std::invalid_argument);
    }

    SECTION("Invalid feature ratio")
    {
        REQUIRE_THROWS_AS(SpectralAnalyzer(frame_duration, sampling_frequency, 0, peak_number), std::invalid_argument);
    }

    SECTION("Invalid number of features")
    {
        REQUIRE_THROWS_AS(SpectralAnalyzer(frame_duration, sampling_frequency, feature_ratio, 0), std::invalid_argument);
    }
}

TEST_CASE("Spectral Analyzer: analyze", "[Spectral Analyzer][analyze]")
{
    const float frame_duration          = .1f   ;
    const uint32_t sampling_frequency   = 100   ;
    const uint32_t feature_ratio        = 5     ;
    const uint8_t peak_number           = 4     ;

    const std::size_t samples_per_frame = static_cast<std::size_t>(frame_duration * sampling_frequency);
    SpectralAnalyzer spectral_analyzer(frame_duration, sampling_frequency, feature_ratio, peak_number);
    std::vector<float> signal;

    auto check_all_elems_size = [samples_per_frame](const std::vector<std::size_t>& v) -> bool
    {
        return (v.size() <= (samples_per_frame / feature_ratio));
    };

    SECTION("Division with no remainder")
    {
        const std::size_t signal_size = 200;
        signal.resize(signal_size);
        std::vector<std::vector<std::size_t>> features = spectral_analyzer.analyze(signal);

        REQUIRE(features.size() == (signal_size / samples_per_frame));
        REQUIRE(std::all_of(features.begin(), features.end(), check_all_elems_size));
    }

    SECTION("Division with non-zero remainder")
    {
        const std::size_t signal_size = 205;
        signal.resize(signal_size);
        std::vector<std::vector<std::size_t>> features = spectral_analyzer.analyze(signal);

        REQUIRE(features.size() == (signal_size / samples_per_frame));
        REQUIRE(std::all_of(features.begin(), features.end(), check_all_elems_size));
    }

    SECTION("Zero number of features")
    {
        const std::size_t signal_size = 5;
        signal.resize(signal_size);
        std::vector<std::vector<std::size_t>> features = spectral_analyzer.analyze(signal);

        REQUIRE(features.size() == (signal_size / samples_per_frame));
        REQUIRE(std::all_of(features.begin(), features.end(), check_all_elems_size));
    }
}
