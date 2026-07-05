#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <chrono>
#include <queue>
#include <vector>
#include <unordered_map>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <cstddef>
#include <optional>
#include <functional>
#include "rq_status_enum.hpp"

template <typename map_value_t>
class Scheduler
{
private:
    using op_time_t = std::chrono::steady_clock::time_point;

    typedef enum
    {
        FSM_IDLE = 0,
        FSM_WORK    ,
        FSM_SAVE    ,
    } fsm_state_t;

    const uint64_t max_rqs_;
    uint64_t job_id_;
    std::queue<std::pair<uint64_t, std::string>> requests_;
    std::unordered_map<uint64_t, map_value_t> op_map_;

    bool keep_running_;
    const std::chrono::minutes expire_mins_;
    std::vector<std::thread> thread_pool_;
    std::condition_variable cv_;
    std::mutex mtx_;

    void _threadRoutine(void);

public:
    using work_fn_sig_t = std::optional<std::string>(const std::string&);
    using save_fn_sig_t = void(map_value_t&, const std::optional<std::string>&);

private:
    std::function<work_fn_sig_t> work_fn_;
    std::function<save_fn_sig_t> save_fn_;

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
    map_value_t getJobResult(const uint64_t job_id) const;
};

template <typename map_value_t>
Scheduler<map_value_t>::Scheduler(  std::function<work_fn_sig_t> work_fn    ,
                                    std::function<save_fn_sig_t> save_fn    ,
                                    const uint64_t max_rqs                  ,
                                    const std::chrono::minutes expire_mins  ,
                                    const uint64_t max_threads              ):
    max_rqs_(max_rqs)           ,
    job_id_(0)                  ,
    keep_running_(false)        ,
    expire_mins_(expire_mins)   ,
    work_fn_(work_fn)           ,
    save_fn_(save_fn)
{
    std::cout << max_threads << std::endl;
    thread_pool_.resize(max_threads);
}

template <typename map_value_t>
Scheduler<map_value_t>::~Scheduler(void)
{
    if(keep_running_)
        end();
}

template <typename map_value_t>
void Scheduler<map_value_t>::run(void)
{
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

        std::cout << "Adding song: " << file_path << std::endl;
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
    fsm_state_t fsm_state = FSM_IDLE;

    using work_fn_ret_t = std::optional<std::string>;

    work_fn_ret_t ret_work;

    while(keep_running_)
    {
        switch (fsm_state)
        {
            case FSM_IDLE:
            {
                // Wait for the condition variable to be raise (which will lead the current thread to be awakened).
                std::unique_lock<std::mutex> lock(mtx_);

                std::cout << "Waiting for cv to be awaken..." << std::endl;

                cv_.wait(lock, [this] { return !requests_.empty() || !keep_running_; });

                if(!keep_running_)
                {
                    std::cout << "No longer operating!!" << std::endl;
                    break;
                }

                job_id = requests_.front().first;
                file_path = requests_.front().second;

                std::cout << "Working: " << file_path << std::endl;

                requests_.pop();

                op_map_[job_id] = {.status = request_status_t::OP_ONGOING};

                fsm_state = FSM_WORK;
            }
            break;

            case FSM_WORK:
            {
                result = request_status_t::OP_OK;

                try
                {
                    // Use constructor/template-provided function here.
                    ret_work = work_fn_(file_path);
                }
                catch(const std::exception& e)
                {
                    std::cerr << e.what() << std::endl;
                    result = request_status_t::OP_ERROR;
                }

                fsm_state = FSM_SAVE;
            }
            break;

            case FSM_SAVE:
            {
                std::lock_guard<std::mutex> lock(mtx_);

                auto& op = op_map_.at(job_id);

                op.status = result;
                op.expire_time = std::chrono::steady_clock::now() + expire_mins_;

                save_fn_(op, ret_work);

                fsm_state = FSM_IDLE;
            }
            break;
        
            default:
            {
                throw std::runtime_error("Unknown status reached: " + std::to_string(static_cast<int>(fsm_state)));
            }
            break;
        }
    }
}

template <typename map_value_t>
bool Scheduler<map_value_t>::hasPendingOps(void) const 
{
    return !requests_.empty();
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
