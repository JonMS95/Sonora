#ifndef SONORA_HPP
#define SONORA_HPP

#include <cstdint>
#include <queue>
#include <string>
#include <chrono>
#include <optional>
#include <functional>
#include "audio_indexer.hpp"
#include "audio_matcher.hpp"
#include "scheduler.hpp"
#include "rq_status_enum.hpp"

// Should we make this a singleton??
class Sonora
{
private:
    using op_time_t = std::chrono::steady_clock::time_point;

    // Indexing-side definitions
    typedef struct
    {
        request_status_t status;
        op_time_t expire_time;
    } index_op_info_t;

    // Indexing-side variables
    AudioIndexer audio_indexer_;

    // Matching-side definitions
    typedef struct
    {
        request_status_t status;
        op_time_t expire_time;
        std::string ret;
    } match_op_info_t;

    // Matching-side variables
    AudioMatcher audio_matcher_;

    // Indexing-side functions
    void _indexRoutine(void);

    // Matching-side functions
    void _matchRoutine(void);

    std::function<std::optional<std::string>(const std::string&)> index_worker_;
    std::function<void(index_op_info_t&, const std::optional<std::string>&)> index_saver_;
    Scheduler<index_op_info_t> index_scheduler_;
    
    std::function<std::optional<std::string>(const std::string&)> match_worker_;
    std::function<void(match_op_info_t&, const std::optional<std::string>&)> match_saver_;
    Scheduler<match_op_info_t> match_scheduler_;

public:
    explicit Sonora(const uint32_t downsmp_freq                                                 ,
                    const std::string& db_path                                                  ,
                    const std::size_t fir_coefs                     = 101                       ,
                    const float frame_duration                      = 0.02f                     ,
                    const uint32_t feature_ratio                    = 20                        ,
                    const uint8_t window_size                       = 5                         ,
                    const uint8_t peak_number                       = 3                         ,
                    const uint64_t max_index_rqs                    = UINT64_MAX                ,
                    const std::chrono::minutes index_expire_mins    = std::chrono::minutes(10)  ,
                    const uint64_t max_match_rqs                    = UINT64_MAX                ,
                    const std::chrono::minutes match_expire_mins    = std::chrono::minutes(10)  ,
                    const uint64_t max_match_threads                = 16                        );
                    
    virtual ~Sonora(void);                

    void run(void);
    void end(void);

    std::optional<uint64_t> index(const std::string& file_path);
    bool hasPendingIndexOps(void) const;
    bool hasOngoingIndexOps(void) const;
    request_status_t getIndexStatus(const uint64_t job_id);

    std::optional<uint64_t> match(const std::string& file_path);
    bool hasPendingMatchOps(void) const;
    bool hasOngoingMatchOps(void) const;
    request_status_t getMatchStatus(const uint64_t job_id);
    std::string getMatchResult(const uint64_t job_id) const;
};

#endif