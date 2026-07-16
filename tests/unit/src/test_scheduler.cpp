#include <catch2/catch.hpp>
#include <filesystem>
#include <stdexcept>
#include <cstddef>
#include <cstdint>
#include <string>
#include <functional>
#include <chrono>
#include <optional>
#include "rq_status_enum.hpp"
#include "scheduler.hpp"

static const std::filesystem::path test_data_dir_path = std::filesystem::path(TEST_DATA_DIR);
static const std::filesystem::path full_samples_dir_path = test_data_dir_path / "samples" / "full_samples";
static const std::filesystem::path samples_path = full_samples_dir_path / "sample_3s_16_khz.wav";

const uint64_t max_rqs                  = 10;
const std::chrono::minutes expire_mins  = std::chrono::minutes{10};
const uint64_t max_threads              = 10;

typedef struct
{
    request_status_t status;
    std::chrono::steady_clock::time_point expire_time;
    std::string ret;
} op_info_t;

static std::function<Scheduler<op_info_t>::work_fn_sig_t> fn_worker =
    [](const std::string& file) -> std::optional<std::string>
    {
        return ("dummy_" + file);
    };

static std::function<Scheduler<op_info_t>::save_fn_sig_t> fn_saver = 
    [](op_info_t& op, const std::optional<std::string>& ret) -> void
    {
        if(ret.has_value())
            op.ret = ret.value();
    };

static std::function<Scheduler<op_info_t>::work_fn_sig_t> fn_sleep =
    [](const std::string& file) -> std::optional<std::string>
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        return ("dummy" + file);
    };

TEST_CASE("Scheduler: Constructor with custom parameters", "[Scheduler][Constructor]")
{
    SECTION("Scheduler instance with proper parameters")
    {
        REQUIRE_NOTHROW(Scheduler<op_info_t>(   fn_worker   ,
                                                fn_saver    ,
                                                max_rqs     ,
                                                expire_mins ,
                                                max_threads ));
    }

    SECTION("Zero expire mins")
    {
        REQUIRE_THROWS_AS(Scheduler<op_info_t>( fn_worker               ,
                                                fn_saver                ,
                                                max_rqs                 ,
                                                std::chrono::minutes{0} ,
                                                max_threads             ),
                                                std::invalid_argument   );
    }

    SECTION("Zero threads for non-null queue")
    {
        REQUIRE_THROWS_AS(Scheduler<op_info_t>( fn_worker   ,
                                                fn_saver    ,
                                                max_rqs     ,
                                                expire_mins ,
                                                0           ),
                                                std::invalid_argument);
    }
}

TEST_CASE("Scheduler: run", "[Scheduler][run]")
{
    SECTION("Execute run method once")
    {
        Scheduler<op_info_t> scheduler( fn_worker   ,
                                        fn_saver    ,
                                        max_rqs     ,
                                        expire_mins ,
                                        max_threads );
        REQUIRE_NOTHROW(scheduler.run());
        REQUIRE(scheduler.isSchedulerRunning());
    }

    SECTION("Execute run method twice")
    {
        Scheduler<op_info_t> scheduler( fn_worker   ,
                                        fn_saver    ,
                                        max_rqs     ,
                                        expire_mins ,
                                        max_threads );
        REQUIRE_NOTHROW(scheduler.run());
        REQUIRE(scheduler.isSchedulerRunning());
        REQUIRE_NOTHROW(scheduler.run());
        REQUIRE(scheduler.isSchedulerRunning());
    }
}

TEST_CASE("Scheduler: end", "[Scheduler][end]")
{
    SECTION("Execute end method once")
    {
        Scheduler<op_info_t> scheduler( fn_worker   ,
                                        fn_saver    ,
                                        max_rqs     ,
                                        expire_mins ,
                                        max_threads );
        scheduler.run();
        REQUIRE_NOTHROW(scheduler.end());
        REQUIRE_FALSE(scheduler.isSchedulerRunning());
    }

    SECTION("Execute end method twice")
    {
        Scheduler<op_info_t> scheduler( fn_worker   ,
                                        fn_saver    ,
                                        max_rqs     ,
                                        expire_mins ,
                                        max_threads );
        scheduler.run();
        REQUIRE_NOTHROW(scheduler.end());
        REQUIRE_FALSE(scheduler.isSchedulerRunning());
        REQUIRE_NOTHROW(scheduler.end());
        REQUIRE_FALSE(scheduler.isSchedulerRunning());
    }

    SECTION("Execute end method with no preceeding run")
    {
        Scheduler<op_info_t> scheduler( fn_worker   ,
                                        fn_saver    ,
                                        max_rqs     ,
                                        expire_mins ,
                                        max_threads );
        REQUIRE_NOTHROW(scheduler.end());
        REQUIRE_FALSE(scheduler.isSchedulerRunning());
    }
}

TEST_CASE("Scheduler: enqueueJob", "[Scheduler][enqueueJob]")
{
    SECTION("Get proper job id")
    {
        Scheduler<op_info_t> scheduler( fn_worker   ,
                                        fn_saver    ,
                                        max_rqs     ,
                                        expire_mins ,
                                        max_threads );
        scheduler.run();

        REQUIRE(scheduler.enqueueJob(std::string(samples_path)) == 0);
    }

    SECTION("Null path")
    {
        Scheduler<op_info_t> scheduler( fn_worker   ,
                                        fn_saver    ,
                                        max_rqs     ,
                                        expire_mins ,
                                        max_threads );
        scheduler.run();     

        REQUIRE_THROWS_AS(scheduler.enqueueJob(""), std::invalid_argument);
    }

    SECTION("File errors")
    {
        Scheduler<op_info_t> scheduler( fn_worker   ,
                                        fn_saver    ,
                                        max_rqs     ,
                                        expire_mins ,
                                        max_threads );
        scheduler.run();              

        SECTION("Non-existing path")
        {
            static const std::string non_existing_path = test_data_dir_path / "samples" / "full_samples" / "non_existing";

            REQUIRE_THROWS_AS(scheduler.enqueueJob(non_existing_path), std::invalid_argument);
        }

        SECTION("Path does not belong to a regular file")
        {
            REQUIRE_THROWS_AS(scheduler.enqueueJob(full_samples_dir_path), std::invalid_argument);
        }
    }

    SECTION("Push to full / null queue")
    {
        Scheduler<op_info_t> scheduler( fn_worker   ,
                                        fn_saver    ,
                                        0           ,
                                        expire_mins ,
                                        0           );
        scheduler.run();

        REQUIRE(scheduler.enqueueJob(std::string(samples_path)) == std::nullopt);
    }
}

TEST_CASE("Scheduler: getJobStatus", "[Scheduler][getJobStatus]")
{
    SECTION("Unknown job")
    {
        Scheduler<op_info_t> scheduler( fn_worker   ,
                                        fn_saver    ,
                                        max_rqs     ,
                                        expire_mins ,
                                        max_threads );
        scheduler.run();

        REQUIRE(scheduler.getJobStatus(0) == request_status_t::OP_UNKNOWN);
    }

    SECTION("Spot ongoing job")
    {
        Scheduler<op_info_t> scheduler( fn_sleep    ,
                                        fn_saver    ,
                                        max_rqs     ,
                                        expire_mins ,
                                        max_threads );
        scheduler.run();

        uint64_t job_id = scheduler.enqueueJob(std::string(samples_path)).value();

        while(scheduler.getJobStatus(job_id) == request_status_t::OP_QUEUED)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        REQUIRE(scheduler.getJobStatus(job_id) == request_status_t::OP_ONGOING);
    }

    SECTION("Spot queued job")
    {
        Scheduler<op_info_t> scheduler( fn_sleep    ,
                                        fn_saver    ,
                                        2           ,
                                        expire_mins ,
                                        1           );
        scheduler.run();

        uint64_t first_job_id = scheduler.enqueueJob(std::string(samples_path)).value();

        while(scheduler.getJobStatus(first_job_id) != request_status_t::OP_ONGOING)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        uint64_t job_id = scheduler.enqueueJob(std::string(samples_path)).value();

        REQUIRE(scheduler.getJobStatus(job_id) == request_status_t::OP_QUEUED);
    }
}

TEST_CASE("Scheduler: hasPendingOps", "[Scheduler][hasPendingOps]")
{
    SECTION("Has pending jobs")
    {
        Scheduler<op_info_t> scheduler( fn_sleep    ,
                                        fn_saver    ,
                                        max_rqs     ,
                                        expire_mins ,
                                        max_threads );
        scheduler.run();

        uint64_t job_id = scheduler.enqueueJob(std::string(samples_path)).value();

        while(scheduler.getJobStatus(job_id) == request_status_t::OP_UNKNOWN)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        REQUIRE(scheduler.hasPendingOps());
    }

    SECTION("Has no pending jobs")
    {
        Scheduler<op_info_t> scheduler( fn_worker   ,
                                        fn_saver    ,
                                        max_rqs     ,
                                        expire_mins ,
                                        max_threads );
        scheduler.run();

        uint64_t job_id = scheduler.enqueueJob(std::string(samples_path)).value();

        while(  (scheduler.getJobStatus(job_id) == request_status_t::OP_UNKNOWN ) ||
                (scheduler.getJobStatus(job_id) == request_status_t::OP_QUEUED  ))
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        REQUIRE_FALSE(scheduler.hasPendingOps());
    }
}

TEST_CASE("Scheduler: hasOngoingOps", "[Scheduler][hasOngoingOps]")
{
    SECTION("Has ongoing jobs")
    {
        Scheduler<op_info_t> scheduler( fn_sleep    ,
                                        fn_saver    ,
                                        max_rqs     ,
                                        expire_mins ,
                                        max_threads );
        scheduler.run();

        uint64_t job_id = scheduler.enqueueJob(std::string(samples_path)).value();

        while((scheduler.getJobStatus(job_id) == request_status_t::OP_UNKNOWN) || (scheduler.getJobStatus(job_id) == request_status_t::OP_QUEUED))
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        REQUIRE(scheduler.hasOngoingOps());
    }

    SECTION("Has no ongoing jobs")
    {
        Scheduler<op_info_t> scheduler( fn_worker   ,
                                        fn_saver    ,
                                        max_rqs     ,
                                        expire_mins ,
                                        max_threads );
        scheduler.run();

        uint64_t job_id = scheduler.enqueueJob(std::string(samples_path)).value();

        while(  (scheduler.getJobStatus(job_id) == request_status_t::OP_UNKNOWN ) ||
                (scheduler.getJobStatus(job_id) == request_status_t::OP_QUEUED  ) ||
                (scheduler.getJobStatus(job_id) == request_status_t::OP_ONGOING ))
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        REQUIRE_FALSE(scheduler.hasOngoingOps());
    }
}

TEST_CASE("Scheduler: isSchedulerRunning", "[Scheduler][isSchedulerRunning]")
{
    Scheduler<op_info_t> scheduler( fn_worker   ,
                                    fn_saver    ,
                                    max_rqs     ,
                                    expire_mins ,
                                    max_threads );
    
    SECTION("Is running after run is called")
    {
        scheduler.run();

        REQUIRE(scheduler.isSchedulerRunning());
    }

    SECTION("Is not running if run is not called")
    {
        REQUIRE_FALSE(scheduler.isSchedulerRunning());
    }

    SECTION("Is not running after end is called")
    {
        scheduler.run();
        scheduler.end();

        REQUIRE_FALSE(scheduler.isSchedulerRunning());
    }
}

TEST_CASE("Scheduler: getJobResult", "[Scheduler][getJobResult]")
{
    SECTION("Do the whole job then get the result")
    {
        Scheduler<op_info_t> scheduler( fn_worker   ,
                                        fn_saver    ,
                                        max_rqs     ,
                                        expire_mins ,
                                        max_threads );

        const std::string& path_str = std::string(samples_path);

        scheduler.run();
        uint64_t job_id = scheduler.enqueueJob(path_str).value();

        while(scheduler.getJobStatus(job_id) != request_status_t::OP_OK)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        REQUIRE(scheduler.getJobResult(job_id).ret == ("dummy_" + path_str));
    }

    SECTION("Do the job but ask for the result early")
    {
        Scheduler<op_info_t> scheduler( fn_sleep    ,
                                        fn_saver    ,
                                        max_rqs     ,
                                        expire_mins ,
                                        max_threads );

        const std::string& path_str = std::string(samples_path);

        scheduler.run();
        uint64_t job_id = scheduler.enqueueJob(path_str).value();
        
        REQUIRE_FALSE(scheduler.getJobStatus(job_id) == request_status_t::OP_OK);
        REQUIRE(scheduler.getJobResult(job_id).ret == "");
    }
}
