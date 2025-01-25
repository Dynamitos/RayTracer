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

void ThreadPool::cancel()
{
  {
    std::unique_lock l(queueLock);
    numRemaining = numRunning;
  }
  while (true)
  {
    std::unique_lock l(queueLock);
    if (taskQueue.empty())
      return;
    completedCV.wait(l);
  }
}

void ThreadPool::runBatch(Batch&& batch)
{
  {
    std::unique_lock l(queueLock);
    numRemaining = batch.jobs.size();
    taskQueue.push_back(batch);
    queueCV.notify_all();
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
    Task job;
    {
      std::unique_lock l(queueLock);
      if (taskQueue.empty() || taskQueue.front().jobs.empty())
      {
        queueCV.wait(l);
        continue;
      }
      job = taskQueue.front().jobs.front();
      taskQueue.front().jobs.pop_front();
      numRunning++;
    }
    job.handle();
    {
      std::unique_lock l(queueLock);
      numRemaining--;
      numRunning--;
      if (numRemaining == 0)
      {
        taskQueue.pop_front();
        completedCV.notify_all();
      }
    }
  }
}
