#include <cstddef>
#include <queue>
#include <string>
#include <optional>
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
                const std::chrono::minutes index_expire_mins,
                const uint64_t max_match_rqs                ,
                const std::chrono::minutes match_expire_mins,
                const uint64_t max_match_threads            ):
    audio_indexer_(AudioIndexer(downsmp_freq    ,
                                db_path         ,
                                fir_coefs       ,
                                frame_duration  ,
                                feature_ratio   ,
                                window_size     ,
                                peak_number     )),
    max_index_rqs_(max_index_rqs)               ,
    index_job_id_(0)                            ,
    keep_index_running_(false)                  ,
    index_expire_mins_(index_expire_mins)       ,
    audio_matcher_(AudioMatcher(downsmp_freq    ,
                                db_path         ,
                                fir_coefs       ,
                                frame_duration  ,
                                feature_ratio   ,
                                window_size     ,
                                peak_number     )),
    max_match_rqs_(max_match_rqs)               ,
    match_job_id_(0)                            ,
    keep_match_running_(false)                  ,
    match_expire_mins_(match_expire_mins)
{
    std::cout << max_match_threads << std::endl;
    match_thread_pool_.resize(max_match_threads);
}

Sonora::~Sonora(void)
{
    if(keep_index_running_ || keep_match_running_)
        end();
}

void Sonora::run(void)
{
    keep_index_running_ = true;
    index_thread_ = std::thread(&Sonora::_indexRoutine, this);
    
    keep_match_running_ = true;
    for(std::thread& t : match_thread_pool_)
        t = std::thread(&Sonora::_matchRoutine, this);
}

void Sonora::end(void)
{
    keep_index_running_ = false;
    cv_index_.notify_one();
    index_thread_.join();

    keep_match_running_ = false;
    cv_match_.notify_all();
    for(std::thread& match_thread : match_thread_pool_)
        match_thread.join();
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

    // Init a scope so that the mutex is unlocked by itself.
    {
        std::lock_guard<std::mutex> index_lock(mtx_index_);

        std::cout << "Adding song to index: " << file_path << std::endl;
        index_requests_.push({job_id, file_path});
        index_op_map_[job_id].status = INDEX_OP_QUEUED;
    }

    cv_index_.notify_one();

    return job_id;
}

void Sonora::_indexRoutine(void)
{
    uint64_t job_id;
    std::string file_path;
    index_rq_status_t index_result;
    index_fsm_t index_fsm_state = INDEX_FSM_IDLE;

    while(keep_index_running_)
    {
        switch (index_fsm_state)
        {
            case INDEX_FSM_IDLE:
            {
                // Wait for the condition variable to be raise (which will lead the current thread to be awakened).
                std::unique_lock<std::mutex> lock(mtx_index_);

                std::cout << "Waiting for cv to be awaken..." << std::endl;

                cv_index_.wait(lock, [this] { return !index_requests_.empty() || !keep_index_running_; });

                if(!keep_index_running_)
                {
                    std::cout << "No longer indexing!!" << std::endl;
                    break;
                }

                job_id = index_requests_.front().first;
                file_path = index_requests_.front().second;

                std::cout << "Indexing: " << file_path << std::endl;

                index_requests_.pop();

                index_op_map_[job_id] = {.status = INDEX_OP_ONGOING};

                index_fsm_state = INDEX_FSM_INDEX;
            }
            break;

            case INDEX_FSM_INDEX:
            {
                index_result = INDEX_OP_OK;

                try
                {
                    audio_indexer_.index(file_path);
                }
                catch(const std::exception& e)
                {
                    std::cerr << e.what() << std::endl;
                    index_result = INDEX_OP_ERROR;
                }

                index_fsm_state = INDEX_FSM_SAVE;
            }
            break;

            case INDEX_FSM_SAVE:
            {
                std::lock_guard<std::mutex> lock(mtx_index_);
                index_op_map_.at(job_id) = {.status = index_result, .expire_time = std::chrono::steady_clock::now() + index_expire_mins_};
            
                index_fsm_state = INDEX_FSM_IDLE;
            }
            break;
        
            default:
            {
                throw std::runtime_error("Unknown status reached: " + std::to_string(static_cast<int>(index_fsm_state)));
            }
            break;
        }
    }
}

bool Sonora::hasPendingIndexOps(void)
{
    return !index_requests_.empty();
}

Sonora::index_rq_status_t Sonora::getIndexStatus(const uint64_t job_id)
{
    std::lock_guard<std::mutex> lock(mtx_index_);
    return index_op_map_.at(job_id).status;
}

std::optional<uint64_t> Sonora::match(const std::string& file_path)
{
    // If queue is already full, then exit immediately.
    if(static_cast<uint64_t>(match_requests_.size()) >= max_match_rqs_)
        return std::nullopt;

    uint64_t job_id = match_job_id_;

    // // Check if job_id exists or not. If so, store its status.
    match_rq_status_t job_id_status = match_op_map_.count(job_id) ? match_op_map_.at(job_id).status : MATCH_OP_UNKNOWN;

    // If no index op id was found in the map, then skip this step.
    if(job_id_status != MATCH_OP_UNKNOWN)
    {
        // If it's waiting to be processed (enqueued) or ongoing, then return nullopt.
        if(job_id_status == MATCH_OP_QUEUED || job_id_status == MATCH_OP_ONGOING)
            return std::nullopt;
        
        // Retrieve current time and expire time from the map.
        op_time_t now = std::chrono::steady_clock::now();
        op_time_t match_op_expire = match_op_map_.at(job_id).expire_time;

        // If job is finished but expire time has not been reached yet, then return nullopt.
        if((job_id_status == MATCH_OP_OK || job_id_status == MATCH_OP_ERROR) && match_op_expire > now)
            return std::nullopt;
    }

    match_job_id_ = (match_job_id_ == UINT64_MAX ? 0 : match_job_id_ + 1);

    // Init a scope so that the mutex is unlocked by itself.
    {
        std::lock_guard<std::mutex> match_lock(mtx_match_);

        std::cout << "Adding song to match: " << file_path << std::endl;
        match_requests_.push({job_id, file_path});
        match_op_map_[job_id].status = MATCH_OP_QUEUED;
    }

    cv_match_.notify_one();

    return job_id;
}

void Sonora::_matchRoutine(void)
{
    uint64_t job_id;
    std::string file_path;
    match_rq_status_t match_result;
    match_fsm_t match_fsm_state = MATCH_FSM_IDLE;
    std::string match_db_name = "";

    while(keep_match_running_)
    {
        switch (match_fsm_state)
        {
            case MATCH_FSM_IDLE:
            {
                // Wait for the condition variable to be raise (which will lead the current thread to be awakened).
                std::unique_lock<std::mutex> lock(mtx_match_);

                std::cout << "Waiting for cv to be awaken..." << std::endl;

                cv_match_.wait(lock, [this] { return !match_requests_.empty() || !keep_match_running_; });

                if(!keep_match_running_)
                {
                    std::cout << "No longer matching!!" << std::endl;
                    break;
                }

                job_id = match_requests_.front().first;
                file_path = match_requests_.front().second;

                std::cout << "Matching: " << file_path << std::endl;

                match_requests_.pop();

                match_op_map_[job_id] = {.status = MATCH_OP_ONGOING};

                match_fsm_state = MATCH_FSM_MATCH;
            }
            break;

            case MATCH_FSM_MATCH:
            {
                match_result = MATCH_OP_OK;

                try
                {
                    match_db_name = audio_matcher_.match(file_path);
                }
                catch(const std::exception& e)
                {
                    std::cerr << e.what() << std::endl;
                    match_result = MATCH_OP_ERROR;
                }

                match_fsm_state = MATCH_FSM_SAVE;
            }
            break;

            case MATCH_FSM_SAVE:
            {
                std::lock_guard<std::mutex> lock(mtx_match_);
                match_op_map_.at(job_id) = {.status = match_result, .expire_time = std::chrono::steady_clock::now() + match_expire_mins_, .match_name = match_db_name};
                match_db_name = "";

                match_fsm_state = MATCH_FSM_IDLE;
            }
            break;
        
            default:
            {
                throw std::runtime_error("Unknown status reached: " + std::to_string(static_cast<int>(match_fsm_state)));
            }
            break;
        }
    }
}

bool Sonora::hasPendingMatchOps(void)
{
    return !match_requests_.empty();
}

std::string Sonora::getMatchResult(const uint64_t job_id)
{
    return match_op_map_.at(job_id).match_name;
}
