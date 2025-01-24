#pragma once
#include <condition_variable>
#include <functional>
#include <list>
#include <thread>

class ThreadPool
{
public:
    ThreadPool(uint32_t numWorkers = std::thread::hardware_concurrency());
    ~ThreadPool();
    void addJob(std::function<void()>&& job);
    void waitIdle();
private:
    std::atomic_bool running = true;
    void work();
    std::mutex queueLock;
    std::condition_variable queueCV;
    std::condition_variable completedCV;
    std::list<std::function<void()>> taskQueue;
    std::vector<std::thread> workers;
};