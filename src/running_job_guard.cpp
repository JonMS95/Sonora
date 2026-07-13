#include "running_job_guard.hpp"

RunningJobGuard::RunningJobGuard(std::atomic<uint64_t>& c):
    counter_(c),
    active_(false)
{
}

RunningJobGuard::~RunningJobGuard(void)
{
    if (active_)
        --counter_;
}

RunningJobGuard& RunningJobGuard::operator++()
{
    if (!active_)
    {
        ++counter_;
        active_ = true;
    }

    return *this;
}
