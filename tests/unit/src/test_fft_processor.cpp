#include <catch2/catch.hpp>
#include "fft_processor.hpp"

TEST_CASE("FFT Processor: Constructor with custom parameters", "[FFT Processor][Constructor]")
{
    SECTION("Valid custom parameters")
    {
        REQUIRE_NOTHROW(FFTProcessor(.2f, 8000, 5));
    }

    SECTION("Invalid frame length")
    {
        SECTION("Frame length < 0")
        {
            REQUIRE_THROWS_AS(FFTProcessor(-.2f, 8000, 5), std::invalid_argument);
        }

        SECTION("Frame length == 0")
        {
            REQUIRE_THROWS_AS(FFTProcessor(0.0f, 8000, 5), std::invalid_argument);
        }
    }

    SECTION("Invalid sampling frequency")
    {
        REQUIRE_THROWS_AS(FFTProcessor(.2f, 0, 5), std::invalid_argument);
    }

    SECTION("Invalid feature ratio")
    {
        REQUIRE_THROWS_AS(FFTProcessor(.2f, 8000, 0), std::invalid_argument);
    }
}
