#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <chrono>
#include <queue>
#include <vector>
#include <unordered_map>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <cstdint>
#include <iostream>
#include <optional>
#include <functional>
#include "rq_status_enum.hpp"
#include "running_job_guard.hpp"

template <typename map_value_t>
class Scheduler
{
private:
    using op_time_t = std::chrono::steady_clock::time_point;

    const uint64_t max_rqs_;
    uint64_t job_id_;
    std::queue<std::pair<uint64_t, std::string>> requests_;
    std::unordered_map<uint64_t, map_value_t> op_map_;

    bool keep_running_;
    const std::chrono::minutes expire_mins_;
    std::vector<std::thread> thread_pool_;
    std::condition_variable cv_;
    std::mutex mtx_;

    std::atomic<uint64_t> ongoing_jobs_;

    void _threadRoutine(void);

public:
    using work_fn_sig_t = std::optional<std::string>(const std::string&);
    using save_fn_sig_t = void(map_value_t&, const std::optional<std::string>&);

private:
    std::function<work_fn_sig_t> work_fn_;
    std::function<save_fn_sig_t> save_fn_;

    struct Config
    {
        std::function<work_fn_sig_t> work_fn  ;
        std::function<save_fn_sig_t> save_fn  ;
        const uint64_t max_rqs                ;
        const std::chrono::minutes expire_mins;
        const uint64_t max_threads            ;
    };

    static Config makeConfig(   std::function<work_fn_sig_t> work_fn    ,
                                std::function<save_fn_sig_t> save_fn    ,
                                const uint64_t max_rqs                  ,
                                const std::chrono::minutes expire_mins  ,
                                const uint64_t max_threads              );

    explicit Scheduler(const Config& cfg);

public:
    explicit Scheduler( std::function<work_fn_sig_t> work_fn        ,
                        std::function<save_fn_sig_t> save_fn        ,
                        const uint64_t max_rqs                  = 10,
                        const std::chrono::minutes expire_mins  = 10,
                        const uint64_t max_threads              = 10);
    virtual ~Scheduler(void);

    void run(void);
    void end(void);
    std::optional<uint64_t> enqueueJob(const std::string& file_path);
    request_status_t getJobStatus(const uint64_t job_id);
    bool hasPendingOps(void) const;
    bool hasOngoingOps(void) const;
    bool isSchedulerRunning() const;
    map_value_t getJobResult(const uint64_t job_id) const;
};

template <typename map_value_t>
Scheduler<map_value_t>::Config Scheduler<map_value_t>::makeConfig(  std::function<work_fn_sig_t> work_fn    ,
                                                                    std::function<save_fn_sig_t> save_fn    ,
                                                                    const uint64_t max_rqs                  ,
                                                                    const std::chrono::minutes expire_mins  ,
                                                                    const uint64_t max_threads              )
{
    if(expire_mins == std::chrono::minutes{0})
        throw std::invalid_argument("Minutes to expire cannot be 0");
    
    if(max_rqs != 0 && max_threads == 0)
        throw std::invalid_argument("Number of threads cannot be zero if a non-null queue exists");
    
    Config cfg =
    {
        .work_fn        = work_fn    ,
        .save_fn        = save_fn    ,
        .max_rqs        = max_rqs    ,
        .expire_mins    = expire_mins,
        .max_threads    = max_threads,
    };

    return cfg;
}

template <typename map_value_t>
Scheduler<map_value_t>::Scheduler(const Config& cfg):
    max_rqs_(cfg.max_rqs)           ,
    job_id_(0)                      ,
    keep_running_(false)            ,
    expire_mins_(cfg.expire_mins)   ,
    ongoing_jobs_(0)                ,
    work_fn_(cfg.work_fn)           ,
    save_fn_(cfg.save_fn)           
{
    thread_pool_.resize(cfg.max_threads);
}

template <typename map_value_t>
Scheduler<map_value_t>::Scheduler(  std::function<work_fn_sig_t> work_fn    ,
                                    std::function<save_fn_sig_t> save_fn    ,
                                    const uint64_t max_rqs                  ,
                                    const std::chrono::minutes expire_mins  ,
                                    const uint64_t max_threads              ):
    Scheduler<map_value_t>(Scheduler<map_value_t>::makeConfig(  work_fn     ,
                                                                save_fn     ,
                                                                max_rqs     ,
                                                                expire_mins ,
                                                                max_threads ))
{}

template <typename map_value_t>
Scheduler<map_value_t>::~Scheduler(void)
{
    if(keep_running_)
        end();
}

template <typename map_value_t>
void Scheduler<map_value_t>::run(void)
{
    if(keep_running_)
        return;

    keep_running_ = true;
    for(std::thread& t : thread_pool_)
        t = std::thread(&Scheduler<map_value_t>::_threadRoutine, this);
}

template <typename map_value_t>
void Scheduler<map_value_t>::end(void)
{
    if(!keep_running_)
        return;

    keep_running_ = false;
    cv_.notify_all();
    for(std::thread& th : thread_pool_)
        th.join();
}

template <typename map_value_t>
std::optional<uint64_t> Scheduler<map_value_t>::enqueueJob(const std::string& file_path)
{
    // If queue is already full, then exit immediately.
    if(static_cast<uint64_t>(requests_.size()) >= max_rqs_)
        return std::nullopt;

    uint64_t job_id = job_id_;

    // Check if job_id exists or not. If so, store its status.
    request_status_t job_id_status = op_map_.count(job_id) ? op_map_.at(job_id).status : request_status_t::OP_UNKNOWN;

    // If no op id was found in the map, then skip this step.
    if(job_id_status != request_status_t::OP_UNKNOWN)
    {
        // If it's waiting to be processed (enqueued) or ongoing, then return nullopt.
        if(job_id_status == request_status_t::OP_QUEUED || job_id_status == request_status_t::OP_ONGOING)
            return std::nullopt;
        
        // Retrieve current time and expire time from the map.
        op_time_t now = std::chrono::steady_clock::now();
        op_time_t op_expire = op_map_.at(job_id).expire_time;

        // If job is finished but expire time has not been reached yet, then return nullopt.
        if((job_id_status == request_status_t::OP_OK || job_id_status == request_status_t::OP_ERROR) && op_expire > now)
            return std::nullopt;
    }

    job_id_ = (job_id_ == UINT64_MAX ? 0 : job_id_ + 1);

    // Init a scope so that the mutex is unlocked by itself.
    {
        std::lock_guard<std::mutex> lock(mtx_);

        requests_.push({job_id, file_path});
        op_map_[job_id].status = request_status_t::OP_QUEUED;
    }

    // Notify a thread so that it knows there's already work to do.
    cv_.notify_one();

    return job_id;
}

template <typename map_value_t>
request_status_t Scheduler<map_value_t>::getJobStatus(const uint64_t job_id)
{
    std::lock_guard<std::mutex> lock(mtx_);
    return op_map_.at(job_id).status;
}

template <typename map_value_t>
void Scheduler<map_value_t>::_threadRoutine(void)
{
    uint64_t job_id;
    std::string file_path;
    request_status_t result;

    using work_fn_ret_t = std::optional<std::string>;

    work_fn_ret_t ret_work;

    while(keep_running_)
    {
        RunningJobGuard job_guard(ongoing_jobs_);

        {
            // Wait for the condition variable to be raise (which will lead the current thread to be awakened).
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait(lock, [this] { return !requests_.empty() || !keep_running_; });

            if(!keep_running_)
                break;

            // Increment number of ongoing jobs before popping elements from queue. 
            ++job_guard;

            job_id = requests_.front().first;
            file_path = requests_.front().second;
            requests_.pop();

            op_map_[job_id] = {};
            op_map_[job_id].status = request_status_t::OP_ONGOING;
        }

        try
        {
            // Use constructor/template-provided function here.
            ret_work = work_fn_(file_path);
            result = request_status_t::OP_OK;
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            result = request_status_t::OP_ERROR;
        }

        {
            std::lock_guard<std::mutex> lock(mtx_);

            auto& op = op_map_.at(job_id);

            op.status = result;
            op.expire_time = std::chrono::steady_clock::now() + expire_mins_;

            save_fn_(op, ret_work);
        }
    }
}

template <typename map_value_t>
bool Scheduler<map_value_t>::hasPendingOps(void) const
{
    return !requests_.empty();
}

template <typename map_value_t>
bool Scheduler<map_value_t>::hasOngoingOps(void) const
{
    return (ongoing_jobs_ > 0);
}

template <typename map_value_t>
bool Scheduler<map_value_t>::isSchedulerRunning() const
{
    return keep_running_;
}

template <typename map_value_t>
map_value_t Scheduler<map_value_t>::getJobResult(const uint64_t job_id) const
{
    map_value_t ret = {};
    
    if(op_map_.count(job_id))
        ret = op_map_.at(job_id);

    return ret;
}

#endif
