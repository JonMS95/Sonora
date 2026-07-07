#ifndef RUNNING_JOB_GUARD
#define RUNNING_JOB_GUARD

#include <atomic>
#include <cstdint>

class RunningJobGuard
{
private:
    std::atomic<uint64_t>& counter;

public:
    explicit RunningJobGuard(std::atomic<uint64_t>& c);
    virtual ~RunningJobGuard(void);

    RunningJobGuard(const RunningJobGuard&) = delete;
    RunningJobGuard& operator=(const RunningJobGuard&) = delete;
};

#endif
