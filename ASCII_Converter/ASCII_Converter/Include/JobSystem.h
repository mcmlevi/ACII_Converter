#pragma once
#include <functional>
#include <atomic>
#define RESERVE_CORES 2 // Reserve Cores for system functionality

// A Dispatched job will receive this as function argument:
struct JobDispatchArgs
{
    uint32_t jobIndex;
    uint32_t groupIndex;
};

namespace JobSystem
{
    // Create the internal resources such as worker threads, etc. Call it once when initializing the application.
    void Initialize();

    // Add a job to execute asynchronously. Any idle thread will execute this job.
    void Execute(const std::function<void()>& job, std::atomic<int>* counter);

    // Check if any threads are working currently or not
    bool IsBusy(std::atomic<int>* counter);

    // Wait until all threads become idle
    void Wait(std::atomic<int>* counter);

}


