#include <catch2/catch.hpp>
#include <vector>
#include <stdexcept>
#include "fir_filter.hpp"

TEST_CASE("FIR Filter with default parameters")
{
    REQUIRE_NOTHROW(FIRFilter());
}

TEST_CASE("FIR Filter with custom parameters")
{
    REQUIRE_NOTHROW(FIRFilter(0.25f, 51));
}

TEST_CASE("Cannot have 0 coefficients")
{
    REQUIRE_THROWS_AS(FIRFilter(0.25f, 0), std::invalid_argument);
}

TEST_CASE("0.0 < Cuttof frequency < 0.5")
{
    SECTION("Cuttof frequency <= 0.0 fails")
    {
        REQUIRE_THROWS_AS(FIRFilter(.0f, 51), std::invalid_argument);
        REQUIRE_THROWS_AS(FIRFilter(-.1f, 51), std::invalid_argument);
    }

    SECTION("Cuttof frequency >= 0.5 fails")
    {
        REQUIRE_THROWS_AS(FIRFilter(.5f, 51), std::invalid_argument);
        REQUIRE_THROWS_AS(FIRFilter(.6f, 51), std::invalid_argument);
    }

    SECTION("Cuttof frequency between 0 and 0.5 (non-inclusive) is OK")
    {
        REQUIRE_NOTHROW(FIRFilter(.25f, 51));
    }
}

TEST_CASE("Filtering empty signal returns empty signal")
{
    FIRFilter fir_filter;
    std::vector<float> in_signal;

    std::vector<float> out_signal = fir_filter.applyFIR(in_signal);

    REQUIRE(out_signal.empty());
}

TEST_CASE("Output has the same length as the input")
{
    FIRFilter fir_filter;
    std::vector<float> in_signal = {.1f, .2f, .3f, .4f, .5f};

    std::vector<float> out_signal = fir_filter.applyFIR(in_signal);

    REQUIRE(in_signal.size() == out_signal.size());
}

TEST_CASE("Filtering is deterministic")
{
    FIRFilter fir_filter;
    std::vector<float> in_signal = {.1f, .2f, .3f, .4f, .5f};

    std::vector<float> out_signal_0 = fir_filter.applyFIR(in_signal);
    std::vector<float> out_signal_1 = fir_filter.applyFIR(in_signal);

    REQUIRE(out_signal_0 == out_signal_1);
}

TEST_CASE("Filtering does not modify input signal")
{
    FIRFilter fir_filter;
    std::vector<float> in_signal = {.1f, .2f, .3f, .4f, .5f};
    std::vector<float> in_signal_copy = in_signal;

    fir_filter.applyFIR(in_signal);

    REQUIRE(in_signal == in_signal_copy);
}
