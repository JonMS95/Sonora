#ifndef SONORA_HPP
#define SONORA_HPP

#include <queue>
#include <string>
#include <chrono>
#include <optional>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "audio_indexer.hpp"
#include "audio_matcher.hpp"

// Should we make this a singleton??
class Sonora
{
public:
    typedef enum
    {
        INDEX_OP_QUEUED  = 0,
        INDEX_OP_ONGOING    ,
        INDEX_OP_OK         ,
        INDEX_OP_ERROR      ,
        INDEX_OP_UNKNOWN    ,
    } index_rq_status_t;

    typedef enum
    {
        MATCH_OP_QUEUED  = 0,
        MATCH_OP_ONGOING    ,
        MATCH_OP_OK         ,
        MATCH_OP_ERROR      ,
        MATCH_OP_UNKNOWN    ,
    } match_rq_status_t;

private:
    using op_time_t = std::chrono::steady_clock::time_point;

    // Indexing-side definitions
    typedef struct
    {
        index_rq_status_t status;
        op_time_t expire_time;
    } index_op_info_t;

    typedef enum
    {
        INDEX_FSM_IDLE = 0  ,
        INDEX_FSM_INDEX     ,
        INDEX_FSM_SAVE      ,
    } index_fsm_t;

    // Indexing-side variables
    AudioIndexer audio_indexer_;
    const uint64_t max_index_rqs_;
    
    uint64_t index_job_id_;
    std::queue<std::pair<uint64_t, std::string>> index_requests_;
    std::unordered_map<uint64_t, index_op_info_t> index_op_map_;
    
    bool keep_index_running_;
    const std::chrono::minutes index_expire_mins_;
    std::thread index_thread_;
    std::condition_variable cv_index_;
    std::mutex mtx_index_;

    // Matching-side definitions
    typedef struct
    {
        match_rq_status_t status;
        op_time_t expire_time;
        std::string match_name;
    } match_op_info_t;

    typedef enum
    {
        MATCH_FSM_IDLE = 0  ,
        MATCH_FSM_MATCH     ,
        MATCH_FSM_SAVE      ,
    } match_fsm_t;

    // Matching-side variables
    AudioMatcher audio_matcher_;
    const uint64_t max_match_rqs_;
    
    uint64_t match_job_id_;
    std::queue<std::pair<uint64_t, std::string>> match_requests_;
    std::unordered_map<uint64_t, match_op_info_t> match_op_map_;
    
    bool keep_match_running_;
    const std::chrono::minutes match_expire_mins_;
    std::vector<std::thread> match_thread_pool_;
    std::condition_variable cv_match_;
    std::mutex mtx_match_;

    // Indexing-side functions
    void _indexRoutine(void);

    // Matching-side functions
    void _matchRoutine(void);

public:
    explicit Sonora(const uint32_t downsmp_freq                                                 ,
                    const std::string& db_path                                                  ,
                    const std::size_t fir_coefs                     = 101                       ,
                    const float frame_duration                      = 0.02f                     ,
                    const uint32_t feature_ratio                    = 20                        ,
                    const uint8_t window_size                       = 5                         ,
                    const uint8_t peak_number                       = 3                         ,
                    const uint64_t max_index_rqs                    = 10                        ,
                    const std::chrono::minutes index_expire_mins    = std::chrono::minutes(10)  ,
                    const uint64_t max_match_rqs                    = 10                        ,
                    const std::chrono::minutes match_expire_mins    = std::chrono::minutes(10)  ,
                    const uint64_t max_match_threads                = 10                        );
                    
    virtual ~Sonora(void);                

    void run(void);
    void end(void);

    std::optional<uint64_t> index(const std::string& file_path);
    bool hasPendingIndexOps(void);
    index_rq_status_t getIndexStatus(const uint64_t job_id);

    std::optional<uint64_t> match(const std::string& file_path);
    bool hasPendingMatchOps(void);
    index_rq_status_t getMatchStatus(const uint64_t job_id);
    std::string getMatchResult(const uint64_t job_id);
};

#endif