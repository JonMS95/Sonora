#include <catch2/catch.hpp>
#include "audio_base.hpp"

TEST_CASE("Audio Base: Constructor with default/custom parameters", "[Audio Base][Constructor]")
{
    SECTION("Constructor with default parameters")
    {
        REQUIRE_NOTHROW(AudioBase());
    }

    SECTION("Constructor with custom parameters")
    {
        REQUIRE_NOTHROW(AudioBase(8000, 51, .01f, 5, 5, 5));
    }
}
