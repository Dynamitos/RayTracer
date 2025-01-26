#pragma once
#include <condition_variable>
#include <functional>
#include <list>
#include <thread>
#include "Minimal.h"

struct Batch
{
    std::list<Task> jobs;
};

class ThreadPool
{
public:
    ThreadPool(uint32_t numWorkers = std::thread::hardware_concurrency() - 2);
    ~ThreadPool();
    
    // cancel running jobs
    void cancel();
    void runBatch(Batch&& batch);
private:
    std::atomic_bool running = true;
    void work();
    std::mutex queueLock;
    std::condition_variable queueCV;
    std::condition_variable completedCV;
    uint32_t numRemaining = 0;
    uint32_t numRunning = 0;
    std::list<Batch> taskQueue;
    std::vector<std::thread> workers;
};