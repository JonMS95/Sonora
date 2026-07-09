#include <catch2/catch.hpp>
#include "spectral_analyzer.hpp"

TEST_CASE("Spectral Analyzer: Constructor with custom parameters", "[Spectral Analyzer][Constructor]")
{
    SECTION("Valid custom parameters")
    {
        REQUIRE_NOTHROW(SpectralAnalyzer(.2f, 8000, 5));
    }

    SECTION("Invalid frame length")
    {
        SECTION("Frame length < 0")
        {
            REQUIRE_THROWS_AS(SpectralAnalyzer(-.2f, 8000, 5), std::invalid_argument);
        }

        SECTION("Frame length == 0")
        {
            REQUIRE_THROWS_AS(SpectralAnalyzer(0.0f, 8000, 5), std::invalid_argument);
        }
    }

    SECTION("Invalid sampling frequency")
    {
        REQUIRE_THROWS_AS(SpectralAnalyzer(.2f, 0, 5), std::invalid_argument);
    }

    SECTION("Invalid feature ratio")
    {
        REQUIRE_THROWS_AS(SpectralAnalyzer(.2f, 8000, 0), std::invalid_argument);
    }
}

TEST_CASE("Spectral Analyzer: analyze", "[Spectral Analyzer][analyze]")
{
    // SECTION("Correct number of divisions")
    // {

    // }
}
