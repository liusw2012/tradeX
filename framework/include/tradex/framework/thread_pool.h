#pragma once

#include <functional>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <memory>

namespace tradex {
namespace framework {

// Thread pool for task execution
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    // Prevent copying
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Submit task to pool
    template<typename F>
    auto submit(F&& f) -> std::future<typename std::result_of<F()>::type> {
        using return_type = typename std::result_of<F()>::type;

        auto task_ptr = std::make_shared<std::packaged_task<return_type()>>(
            std::forward<F>(f)
        );

        std::future<return_type> result = task_ptr->get_future();

        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (stopped_) {
                throw std::runtime_error("ThreadPool is stopped");
            }
            tasks_.emplace([task_ptr]() { (*task_ptr)(); });
        }

        condition_.notify_one();
        return result;
    }

    // Get number of active threads
    size_t numThreads() const { return workers_.size(); }

    // Get number of pending tasks
    size_t numPendingTasks() const;

    // Shutdown pool
    void shutdown();
    void shutdownWait();

    // Pause/Resume
    void pause();
    void resume();

private:
    void workerThread();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stopped_;
    std::atomic<bool> paused_;
    std::atomic<int> active_workers_;
};

// Global thread pool instance
ThreadPool& globalThreadPool();

} // namespace framework
} // namespace tradex