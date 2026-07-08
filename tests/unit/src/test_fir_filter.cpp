#include <catch2/catch.hpp>
#include <vector>
#include <stdexcept>
#include "fir_filter.hpp"

TEST_CASE("FIR Filter: Constructor with default/custom parameters", "[FIR Filter][Constructor]")
{
    SECTION("FIR Filter with default parameters")
    {
        REQUIRE_NOTHROW(FIRFilter());
    }

    SECTION("FIR Filter with custom parameters")
    {
        SECTION("Correct parameters")
        {
            REQUIRE_NOTHROW(FIRFilter(0.25f, 51));
        }

        SECTION("0 coefficients")
        {
            REQUIRE_THROWS_AS(FIRFilter(0.25f, 0), std::invalid_argument);
        }

        SECTION("Cutoff frequency")
        {
            SECTION("Cuttof frequency <= 0.0")
            {
                REQUIRE_THROWS_AS(FIRFilter(.0f, 51), std::invalid_argument);
                REQUIRE_THROWS_AS(FIRFilter(-.1f, 51), std::invalid_argument);
            }
        
            SECTION("Cuttof frequency >= 0.5")
            {
                REQUIRE_THROWS_AS(FIRFilter(.5f, 51), std::invalid_argument);
                REQUIRE_THROWS_AS(FIRFilter(.6f, 51), std::invalid_argument);
            }

            SECTION("Cuttof frequency between 0 and 0.5")
            {
                REQUIRE_NOTHROW(FIRFilter(.25f, 51));
            }
        }
    }
}

TEST_CASE("FIR Filter: Filtering method", "[Fir Filter][fir_filter]")
{
    FIRFilter fir_filter;

    SECTION("Filtering empty signal returns empty signal")
    {
        std::vector<float> in_signal;
        std::vector<float> out_signal = fir_filter.applyFIR(in_signal);

        REQUIRE(out_signal.empty());
    }

    SECTION("Output has the same length as the input")
    {
        std::vector<float> in_signal = {.1f, .2f, .3f, .4f, .5f};
        std::vector<float> out_signal = fir_filter.applyFIR(in_signal);

        REQUIRE(in_signal.size() == out_signal.size());
    }

    SECTION("Filtering is deterministic")
    {
        std::vector<float> in_signal = {.1f, .2f, .3f, .4f, .5f};
        std::vector<float> out_signal_0 = fir_filter.applyFIR(in_signal);
        std::vector<float> out_signal_1 = fir_filter.applyFIR(in_signal);

        REQUIRE(out_signal_0 == out_signal_1);
    }

    SECTION("Filtering does not modify input signal")
    {
        FIRFilter fir_filter;
        std::vector<float> in_signal = {.1f, .2f, .3f, .4f, .5f};
        std::vector<float> in_signal_copy = in_signal;

        fir_filter.applyFIR(in_signal);

        REQUIRE(in_signal == in_signal_copy);
    }
}
