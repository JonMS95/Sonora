#include "running_job_guard.hpp"

RunningJobGuard::RunningJobGuard(std::atomic<uint64_t>& c):
    counter(c)
{
    ++counter;
}

RunningJobGuard::~RunningJobGuard(void)
{
    --counter;
}
