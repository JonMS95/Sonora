#include <catch2/catch.hpp>
#include <atomic>
#include <cstdint>
#include <type_traits>
#include "running_job_guard.hpp"

TEST_CASE("RunningJobGuard: Constructor with proper input parameter", "[RunningJobGuard][Constructor]")
{
    std::atomic<uint64_t> x = 0;

    REQUIRE_NOTHROW(RunningJobGuard(x));
    REQUIRE(x == 0);
}

TEST_CASE("RunningJobGuard: Counter management", "[RunningJobGuard][counter]")
{
    std::atomic<uint64_t> x = 0;

    SECTION("Inactive guard does not increment counter")
    {
        RunningJobGuard guard(x);
        REQUIRE(x == 0);
    }

    SECTION("Increment counter")
    {
        RunningJobGuard guard(x);

        ++guard;

        REQUIRE(x == 1);
    }

    SECTION("Decrement counter after activation")
    {
        {
            RunningJobGuard guard(x);

            ++guard;
            REQUIRE(x == 1);
        }

        REQUIRE(x == 0);
    }

    SECTION("Multiple guards accumulate")
    {
        RunningJobGuard guard_0(x);
        RunningJobGuard guard_1(x);

        ++guard_0;
        REQUIRE(x == 1);

        ++guard_1;
        REQUIRE(x == 2);
    }

    SECTION("Nested scopes increment/decrement correctly")
    {
        {
            RunningJobGuard guard_0(x);
            ++guard_0;

            REQUIRE(x == 1);

            {
                RunningJobGuard guard_1(x);
                ++guard_1;

                REQUIRE(x == 2);
            }

            REQUIRE(x == 1);
        }

        REQUIRE(x == 0);
    }

    SECTION("Multiple increments on the same guard do not double count")
    {
        {
            RunningJobGuard guard(x);

            ++guard;
            ++guard;
            ++guard;

            REQUIRE(x == 1);
        }

        REQUIRE(x == 0);
    }
}

TEST_CASE("RunningJobGuard: RunningJobGuard is not copyable", "[RunningJobGuard][not copyable]")
{
    STATIC_REQUIRE(!std::is_copy_constructible<RunningJobGuard>::value);
    STATIC_REQUIRE(!std::is_copy_assignable<RunningJobGuard>::value);
}