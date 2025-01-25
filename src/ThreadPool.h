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
    ThreadPool(uint32_t numWorkers = std::thread::hardware_concurrency());
    ~ThreadPool();
    void runBatch(Batch&& batch);
private:
    std::atomic_bool running = true;
    void work();
    std::mutex queueLock;
    std::condition_variable queueCV;
    std::condition_variable completedCV;
    uint32_t numRemaining;
    std::list<Batch> taskQueue;
    std::vector<std::thread> workers;
};