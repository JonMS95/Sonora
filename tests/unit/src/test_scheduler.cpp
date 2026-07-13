#include <catch2/catch.hpp>
#include <filesystem>
#include <stdexcept>
#include <cstddef>
#include <cstdint>
#include <string>
#include <functional>
#include <chrono>
#include <optional>
// #include "test_db_helper.hpp"
#include "audio_indexer.hpp"
#include "audio_matcher.hpp"
#include "rq_status_enum.hpp"
#include "scheduler.hpp"

static const std::filesystem::path test_data_dir_path = std::filesystem::path(TEST_DATA_DIR);
static const std::filesystem::path db_dir_path = test_data_dir_path / "db";
static const std::filesystem::path full_samples_dir_path = test_data_dir_path / "samples" / "full_samples";

static const std::string& samples_db_path = std::string(db_dir_path / "sample_fingerprints.db");

static const std::string& dummy_db_base = "dummy_test_audio_indexer.db";
static const std::string& dummy_db_path = std::string(db_dir_path / dummy_db_base);
static const std::string& dummy_db_wal_path = std::string(db_dir_path / (dummy_db_base + "-wal"));
static const std::string& dummy_db_shm_path = std::string(db_dir_path / (dummy_db_base + "-shm"));

static const uint32_t downsmp_freq     = 8000   ;
static const std::size_t fir_coefs     = 51     ;
static const float frame_duration      = .01f   ;
static const uint32_t feature_ratio    = 10     ;
static const uint8_t window_size       = 3      ;
static const uint8_t peak_number       = 3      ;

const uint64_t max_index_rqs                    = 5;
const std::chrono::minutes index_expire_mins    = std::chrono::minutes{10};
const uint64_t max_index_threads                = 1;

const uint64_t max_match_rqs                    = 10;
const std::chrono::minutes match_expire_mins    = std::chrono::minutes{10};
const uint64_t max_match_threads                = 10;

typedef struct
{
    request_status_t status;
    std::chrono::steady_clock::time_point expire_time;
} index_op_info_t;

typedef struct
{
    request_status_t status;
    std::chrono::steady_clock::time_point expire_time;
    std::string ret;
} match_op_info_t;

static  AudioIndexer audio_indexer( downsmp_freq    ,
                                    samples_db_path ,
                                    fir_coefs       ,
                                    frame_duration  ,
                                    feature_ratio   ,
                                    window_size     ,
                                    peak_number     );

static  std::function<Scheduler<index_op_info_t>::work_fn_sig_t> index_worker =
            [](const std::string& file) -> std::optional<std::string>
            {
                audio_indexer.index(file);
                return std::nullopt;
            };

static  std::function<Scheduler<index_op_info_t>::save_fn_sig_t> index_saver = 
            [](index_op_info_t&, const std::optional<std::string>&) -> void {};

static  AudioMatcher audio_matcher( downsmp_freq    ,
                                    samples_db_path ,
                                    fir_coefs       ,
                                    frame_duration  ,
                                    feature_ratio   ,
                                    window_size     ,
                                    peak_number     );

static  std::function<Scheduler<match_op_info_t>::work_fn_sig_t> match_worker =
    [](const std::string& file) -> std::optional<std::string>
    {
        return audio_matcher.match(file);
    };

static  std::function<Scheduler<match_op_info_t>::save_fn_sig_t> match_saver = 
    [](match_op_info_t& op, const std::optional<std::string>& ret) -> void
    {
        if(ret.has_value())
            op.ret = ret.value();
    };

TEST_CASE("Scheduler: Constructor with custom parameters", "[Scheduler][Constructor]")
{
    SECTION("Index-oriented Scheduler instance")
    {
        SECTION("Index-oriented Scheduler instance with proper parameters")
        {
            REQUIRE_NOTHROW(Scheduler<index_op_info_t>( index_worker            ,
                                                        index_saver             ,
                                                        max_index_rqs           ,
                                                        index_expire_mins       ,
                                                        max_index_threads       ));
        }

        SECTION("Index-oriented Scheduler instance with wrong parameters")
        {
            SECTION("Zero expire mins")
            {
                REQUIRE_THROWS_AS(Scheduler<index_op_info_t>(   index_worker            ,
                                                                index_saver             ,
                                                                max_index_rqs           ,
                                                                std::chrono::minutes{0} ,
                                                                max_index_threads       ),
                                                                std::invalid_argument   );
            }

            SECTION("Zero threads for non-null queue")
            {
                REQUIRE_THROWS_AS(Scheduler<index_op_info_t>(   index_worker            ,
                                                                index_saver             ,
                                                                max_index_rqs           ,
                                                                index_expire_mins       ,
                                                                0                       ),
                                                                std::invalid_argument   );
            }
        }
    }

    SECTION("Match-oriented Scheduler instance")
    {
        SECTION("Match-oriented Scheduler instance with proper parameters")
        {
            REQUIRE_NOTHROW(Scheduler<match_op_info_t>( match_worker            ,
                                                        match_saver             ,
                                                        max_match_rqs           ,
                                                        match_expire_mins       ,
                                                        max_match_threads       ));
        }

        SECTION("Match-oriented Scheduler instance with wrong parameters")
        {
            SECTION("Zero expire mins")
            {
                REQUIRE_THROWS_AS(Scheduler<match_op_info_t>(   match_worker            ,
                                                                match_saver             ,
                                                                max_match_rqs           ,
                                                                std::chrono::minutes{0} ,
                                                                max_match_threads       ),
                                                                std::invalid_argument   );
            }

            SECTION("Zero threads for non-null queue")
            {
                REQUIRE_THROWS_AS(Scheduler<match_op_info_t>(   match_worker            ,
                                                                match_saver             ,
                                                                max_match_rqs           ,
                                                                match_expire_mins       ,
                                                                0                       ),
                                                                std::invalid_argument   );
            }
        }
    }
}

TEST_CASE("Scheduler: run", "[Scheduler][run]")
{
    SECTION("Index-oriented scheduler")
    {
        SECTION("Execute run method once")
        {
            Scheduler<index_op_info_t> index_scheduler( index_worker            ,
                                                        index_saver             ,
                                                        max_index_rqs           ,
                                                        index_expire_mins       ,
                                                        max_index_threads       );
            REQUIRE_NOTHROW(index_scheduler.run());
            REQUIRE(index_scheduler.isSchedulerRunning());
        }

        SECTION("Execute run method twice")
        {
            Scheduler<index_op_info_t> index_scheduler( index_worker            ,
                                                        index_saver             ,
                                                        max_index_rqs           ,
                                                        index_expire_mins       ,
                                                        max_index_threads       );
            REQUIRE_NOTHROW(index_scheduler.run());
            REQUIRE(index_scheduler.isSchedulerRunning());
            REQUIRE_NOTHROW(index_scheduler.run());
            REQUIRE(index_scheduler.isSchedulerRunning());
        }
    }

    SECTION("Match-oriented scheduler")
    {
        SECTION("Execute run method once")
        {
            Scheduler<match_op_info_t> match_scheduler( match_worker            ,
                                                        match_saver             ,
                                                        max_match_rqs           ,
                                                        match_expire_mins       ,
                                                        max_match_threads       );
            REQUIRE_NOTHROW(match_scheduler.run());
            REQUIRE(match_scheduler.isSchedulerRunning());
        }

        SECTION("Execute run method twice")
        {
            Scheduler<match_op_info_t> match_scheduler( match_worker            ,
                                                        match_saver             ,
                                                        max_match_rqs           ,
                                                        match_expire_mins       ,
                                                        max_match_threads       );
            REQUIRE_NOTHROW(match_scheduler.run());
            REQUIRE(match_scheduler.isSchedulerRunning());
            REQUIRE_NOTHROW(match_scheduler.run());
            REQUIRE(match_scheduler.isSchedulerRunning());
        }
    }
}

TEST_CASE("Scheduler: end", "[Scheduler][end]")
{
    SECTION("Index-oriented scheduler")
    {
        SECTION("Execute end method once")
        {
            Scheduler<index_op_info_t> index_scheduler( index_worker            ,
                                                        index_saver             ,
                                                        max_index_rqs           ,
                                                        index_expire_mins       ,
                                                        max_index_threads       );
            REQUIRE_NOTHROW(index_scheduler.run());
            REQUIRE(index_scheduler.isSchedulerRunning());
            REQUIRE_NOTHROW(index_scheduler.end());
            REQUIRE_FALSE(index_scheduler.isSchedulerRunning());
        }

        SECTION("Execute end method twice")
        {
            Scheduler<index_op_info_t> index_scheduler( index_worker            ,
                                                        index_saver             ,
                                                        max_index_rqs           ,
                                                        index_expire_mins       ,
                                                        max_index_threads       );
            REQUIRE_NOTHROW(index_scheduler.run());
            REQUIRE(index_scheduler.isSchedulerRunning());
            REQUIRE_NOTHROW(index_scheduler.end());
            REQUIRE_FALSE(index_scheduler.isSchedulerRunning());
            REQUIRE_NOTHROW(index_scheduler.end());
            REQUIRE_FALSE(index_scheduler.isSchedulerRunning());
        }

        SECTION("Execute end method with no preceeding run")
        {
            Scheduler<index_op_info_t> index_scheduler( index_worker            ,
                                                        index_saver             ,
                                                        max_index_rqs           ,
                                                        index_expire_mins       ,
                                                        max_index_threads       );
            REQUIRE_NOTHROW(index_scheduler.end());
            REQUIRE_FALSE(index_scheduler.isSchedulerRunning());
        }
    }

    SECTION("Match-oriented scheduler")
    {
        SECTION("Execute end method once")
        {
            Scheduler<match_op_info_t> match_scheduler( match_worker            ,
                                                        match_saver             ,
                                                        max_match_rqs           ,
                                                        match_expire_mins       ,
                                                        max_match_threads       );
            REQUIRE_NOTHROW(match_scheduler.run());
            REQUIRE(match_scheduler.isSchedulerRunning());
            REQUIRE_NOTHROW(match_scheduler.end());
            REQUIRE_FALSE(match_scheduler.isSchedulerRunning());
        }

        SECTION("Execute end method twice")
        {
            Scheduler<match_op_info_t> match_scheduler( match_worker            ,
                                                        match_saver             ,
                                                        max_match_rqs           ,
                                                        match_expire_mins       ,
                                                        max_match_threads       );
            REQUIRE_NOTHROW(match_scheduler.run());
            REQUIRE(match_scheduler.isSchedulerRunning());
            REQUIRE_NOTHROW(match_scheduler.end());
            REQUIRE_FALSE(match_scheduler.isSchedulerRunning());
            REQUIRE_NOTHROW(match_scheduler.end());
            REQUIRE_FALSE(match_scheduler.isSchedulerRunning());
        }

        SECTION("Execute end method with no preceeding run")
        {
            Scheduler<match_op_info_t> match_scheduler( match_worker            ,
                                                        match_saver             ,
                                                        max_match_rqs           ,
                                                        match_expire_mins       ,
                                                        max_match_threads       );
            REQUIRE_NOTHROW(match_scheduler.end());
            REQUIRE_FALSE(match_scheduler.isSchedulerRunning());
        }
    }
}

// TEST_CASE("Scheduler: enqueueJob", "[Scheduler][enqueueJob]")
// {
//     SECTION("Index-oriented scheduler")
//     {
//         SECTION("Push to full / null queue")
//         {
//             Scheduler<index_op_info_t> index_scheduler( index_worker            ,
//                                                         index_saver             ,
//                                                         0                       ,
//                                                         index_expire_mins       ,
//                                                         0                       );
//             REQUIRE_NOTHROW(index_scheduler.run());
            
//         }
//     }

//     SECTION("Match-oriented scheduler")
//     {
//         SECTION("Execute end method once")
//         {
//             Scheduler<match_op_info_t> match_scheduler( match_worker            ,
//                                                         match_saver             ,
//                                                         max_match_rqs           ,
//                                                         match_expire_mins       ,
//                                                         max_match_threads       );
//             REQUIRE_NOTHROW(match_scheduler.run());
//             REQUIRE_NOTHROW(match_scheduler.end());
//         }
//     }
// }
