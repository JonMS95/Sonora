#ifndef SONORA_HPP
#define SONORA_HPP

#include <queue>
#include <string>
#include <thread>
#include <chrono>
#include <optional>
#include <unordered_map>
#include "audio_indexer.hpp"
#include "audio_matcher.hpp"

// Should we make this a singleton??
class Sonora
{
private:
    using op_time_t = std::chrono::steady_clock::time_point;

    typedef enum
    {
        INDEX_OP_QUEUED  = 0,
        INDEX_OP_ONGOING    ,
        INDEX_OP_OK         ,
        INDEX_OP_ERROR      ,
        INDEX_OP_UNKNOWN    ,
    } index_rq_status_t;

    typedef struct
    {
        index_rq_status_t status;
        op_time_t expire_time;
    } index_op_info_t;

    typedef enum
    {
        INDEX_FSM_IDLE = 0  ,
        INDEX_FSM_BUSY      ,
    } index_fsm_t;

    // Indexing variables
    AudioIndexer audio_indexer_;
    const uint64_t max_index_rqs_;
    uint64_t index_job_id_;
    std::queue<std::pair<uint64_t, std::string>> index_requests_;
    std::unordered_map<uint64_t, index_op_info_t> index_op_map_;
    index_fsm_t index_fsm_state_;
    bool keep_running_;
    const std::chrono::minutes index_expire_mins_;
    std::thread index_thread_;

    // Indexing functions
    void _indexRoutine(void);

public:
    explicit Sonora(const uint32_t downsmp_freq                                                 ,
                    const std::string& db_path                                                  ,
                    const std::size_t fir_coefs                     = 101                       ,
                    const float frame_duration                      = 0.02f                     ,
                    const uint32_t feature_ratio                    = 20                        ,
                    const uint8_t window_size                       = 5                         ,
                    const uint8_t peak_number                       = 3                         ,
                    const uint64_t max_index_rqs                    = 10                        ,
                    const std::chrono::minutes index_expire_mins    = std::chrono::minutes(10)  );
                    
    virtual ~Sonora(void);                

    void run(void);
    void end(void);

    std::optional<uint64_t> index(const std::string& file_path);
};

#endif