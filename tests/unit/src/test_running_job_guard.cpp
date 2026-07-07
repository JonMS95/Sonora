#include <catch2/catch.hpp>
#include <stdexcept>
#include <atomic>
#include <cstdint>
#include <type_traits>
#include "running_job_guard.hpp"

TEST_CASE("Constructor with proper input parameter")
{
    std::atomic<uint64_t> x = 0;
    REQUIRE_NOTHROW(RunningJobGuard(x));
}

TEST_CASE("Counter management")
{
    std::atomic<uint64_t> x = 0;

    SECTION("Increment counter")
    {
        RunningJobGuard guard(x);
        REQUIRE(x == 1);
    }

    SECTION("Decrement counter")
    {
        {
            RunningJobGuard guard(x);
            REQUIRE(x == 1);
        }

        REQUIRE(x == 0);
    }

    SECTION("Multiple guards accumulate")
    {
        RunningJobGuard guard_0(x);
        REQUIRE(x == 1);

        RunningJobGuard guard_1(x);
        REQUIRE(x == 2);
    }

    SECTION("Nested scopes increment/decrement correctly")
    {
        {
            RunningJobGuard guard_0(x);
            REQUIRE(x == 1);

            {
                RunningJobGuard guard_1(x);
                REQUIRE(x == 2);
            }

            REQUIRE(x == 1);
        }

        REQUIRE(x == 0);
    }
}

TEST_CASE("RunningJobGuard is not copyable")
{
    STATIC_REQUIRE(!std::is_copy_constructible<RunningJobGuard>::value);
    STATIC_REQUIRE(!std::is_copy_assignable<RunningJobGuard>::value);
}