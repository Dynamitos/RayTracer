#pragma once
#include <condition_variable>
#include <functional>
#include <list>
#include <thread>
#include "Minimal.h"
#include <coroutine>

struct Task
{
  struct promise_type
  {
    Task get_return_object() { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };
  std::coroutine_handle<promise_type> handle;
};
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