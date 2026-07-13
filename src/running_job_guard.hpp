#ifndef RUNNING_JOB_GUARD_HPP
#define RUNNING_JOB_GUARD_HPP

#include <atomic>
#include <cstdint>

class RunningJobGuard
{
private:
    std::atomic<uint64_t>& counter_;
    bool active_;

public:
    explicit RunningJobGuard(std::atomic<uint64_t>& c);
    ~RunningJobGuard(void);

    RunningJobGuard& operator++();

    RunningJobGuard(const RunningJobGuard&) = delete;
    RunningJobGuard& operator=(const RunningJobGuard&) = delete;
};

#endif