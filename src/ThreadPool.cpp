#include "ThreadPool.h"
#include <mutex>

ThreadPool::ThreadPool(uint32_t numThreads)
{
    for (uint32_t i = 0; i < numThreads; ++i)
    {
        workers.push_back(std::thread(&ThreadPool::work, this));
    }
}

ThreadPool::~ThreadPool()
{
    running.store(false);
    {
        std::unique_lock l(queueLock);
        queueCV.notify_all();
    }
    for (auto& worker : workers)
    {
        worker.join();
    }
}

void ThreadPool::runBatch(Batch&& batch)
{
    {
        std::unique_lock l(queueLock);
        taskQueue.push_back(batch);
        queueCV.notify_one();
    }
    while (true)
    {
        std::unique_lock l(queueLock);
        if (taskQueue.empty())
            return;
        completedCV.wait(l);
    }
}

void ThreadPool::work()
{
    while (running)
    {
        std::function<void()> job;
        {
            std::unique_lock l(queueLock);
            if (taskQueue.front().jobs.empty())
            {
                queueCV.wait(l);
                continue;
            }
            job = taskQueue.front().jobs.front();
            taskQueue.front().jobs.pop_front();
        }
        job();
        {
            std::unique_lock l(queueLock);
            if (taskQueue.front().jobs.empty())
            {
                taskQueue.pop_front();
                completedCV.notify_one();
            }
        }
    }
}
