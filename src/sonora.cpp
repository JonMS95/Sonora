#include <cstddef>
#include <queue>
#include <string>
#include <iostream>
#include <stdexcept>
#include "audio_indexer.hpp"
#include "audio_matcher.hpp"
#include "sonora.hpp"

#define DEFAULT_SLEEP_SECS 1

Sonora::Sonora( const uint32_t downsmp_freq                 ,
                const std::string& db_path                  ,
                const std::size_t fir_coefs                 ,
                const float frame_duration                  ,
                const uint32_t feature_ratio                ,
                const uint8_t window_size                   ,
                const uint8_t peak_number                   ,
                const uint64_t max_index_rqs                ,
                const std::chrono::minutes index_expire_mins):
    audio_indexer_(AudioIndexer(downsmp_freq    ,
                                db_path         ,
                                fir_coefs       ,
                                frame_duration  ,
                                feature_ratio   ,
                                window_size     ,
                                peak_number     )),
    max_index_rqs_(max_index_rqs),
    index_job_id_(0),
    index_fsm_state_(INDEX_FSM_IDLE),
    keep_running_(false),
    index_expire_mins_(index_expire_mins)
{}

Sonora::~Sonora(void)
{
    if(keep_running_)
        end();
}

void Sonora::run(void)
{
    keep_running_ = true;
    index_thread_ = std::thread(&Sonora::_indexRoutine, this);
}

void Sonora::end(void)
{
    keep_running_ = false;
    index_thread_.join();
}

std::optional<uint64_t> Sonora::index(const std::string& file_path)
{
    // If queue is already full, then exit immediately.
    if(static_cast<uint64_t>(index_requests_.size()) >= max_index_rqs_)
        return std::nullopt;

    uint64_t job_id = index_job_id_;

    // Check if job_id exists or not. If so, store its status.
    index_rq_status_t job_id_status = index_op_map_.count(job_id) ? index_op_map_.at(job_id).status : INDEX_OP_UNKNOWN;

    // If no index op id was found in the map, then skip this step.
    if(job_id_status != INDEX_OP_UNKNOWN)
    {
        // If it's waiting to be processed (enqueued) or ongoing, then return nullopt.
        if(job_id_status == INDEX_OP_QUEUED || job_id_status == INDEX_OP_ONGOING)
            return std::nullopt;
        
        // Retrieve current time and expire time from the map.
        op_time_t now = std::chrono::steady_clock::now();
        op_time_t index_op_expire = index_op_map_.at(job_id).expire_time;

        // If job is finished but expire time has not been reached yet, then return nullopt.
        if((job_id_status == INDEX_OP_OK || job_id_status == INDEX_OP_ERROR) && index_op_expire > now)
            return std::nullopt;
    }

    index_job_id_ = (index_job_id_ == UINT64_MAX ? 0 : index_job_id_ + 1);

    index_requests_.push({job_id, file_path});
    index_op_map_[job_id].status = INDEX_OP_QUEUED;

    return job_id;
}

void Sonora::_indexRoutine(void)
{
    while(keep_running_)
    {
        switch (index_fsm_state_)
        {
            case INDEX_FSM_IDLE:
            {
                if(index_requests_.empty())
                {
                    std::this_thread::sleep_for(std::chrono::seconds(DEFAULT_SLEEP_SECS));
                    continue;
                }
                
                index_fsm_state_ = INDEX_FSM_BUSY;
            }
            break;

            case INDEX_FSM_BUSY:
            {
                const uint64_t job_id = index_requests_.front().first;
                const std::string file_path = index_requests_.front().second;

                index_requests_.pop();
                index_op_map_[job_id] = {.status = INDEX_OP_ONGOING};

                index_rq_status_t index_result = INDEX_OP_OK;

                try
                {
                    audio_indexer_.index(file_path);
                }
                catch(const std::exception& e)
                {
                    std::cerr << e.what() << std::endl;
                    index_result = INDEX_OP_ERROR;
                }

                index_op_map_[job_id] = {.status = index_result, .expire_time = std::chrono::steady_clock::now() + index_expire_mins_};
            
                index_fsm_state_ = INDEX_FSM_IDLE;
            }
            break;
        
            default:
            {
                throw std::runtime_error("Unknown status reached: " + index_fsm_state_);
            }
            break;
        }
    }
}
