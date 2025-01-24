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
    for(auto& worker : workers)
    {
        worker.join();
    }
}

void ThreadPool::addJob(std::function<void()>&& job)
{
    std::unique_lock l(queueLock);
    taskQueue.push_back(job);
    queueCV.notify_one();
}

void ThreadPool::waitIdle()
{
    while(true)
    {
        std::unique_lock l(queueLock);
        if(taskQueue.empty())
            return;
        completedCV.wait(l);
    }
}

void ThreadPool::work()
{
    while(running)
    {
        std::function<void()> job;
        {
            std::unique_lock l(queueLock);
            if(taskQueue.empty())
            {
                queueCV.wait(l);
                continue;
            }
            job = taskQueue.front();
            taskQueue.pop_front();
        }
        job();
        {
            std::unique_lock l(queueLock);
            completedCV.notify_one();
        }
    }
}

