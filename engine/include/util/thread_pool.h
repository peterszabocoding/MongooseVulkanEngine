#pragma once

// Source: https://www.geeksforgeeks.org/cpp/thread-pool-in-cpp/

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include "util/core.h"

namespace MongooseVK
{
    class ThreadPool {
    public:
        static ThreadPool* Create(size_t num_threads = std::thread::hardware_concurrency()) { return new ThreadPool(num_threads); }
        static ThreadPool* Get() { return instance; }

        // Enqueue task for execution by the thread pool
        void enqueue(std::function<void()> task);

    private:
        // // Constructor to creates a thread pool with given
        // number of threads
        explicit ThreadPool(const size_t num_threads = std::thread::hardware_concurrency());


        // Destructor to stop the thread pool
        ~ThreadPool();

    private:
        static ThreadPool* instance;

        // Vector to store worker threads
        std::vector<std::thread> threads_;

        // Queue of tasks
        std::queue<std::function<void()>> tasks_;

        // Mutex to synchronize access to shared data
        std::mutex queue_mutex_;

        // Condition variable to signal changes in the state of
        // the tasks queue
        std::condition_variable cv_;

        // Flag to indicate whether the thread pool should stop
        // or not
        bool stop_ = false;
    };
}
