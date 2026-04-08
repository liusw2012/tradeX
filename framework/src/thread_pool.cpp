#include "tradex/framework/thread_pool.h"

namespace tradex {
namespace framework {

ThreadPool::ThreadPool(size_t num_threads)
    : stopped_(false), paused_(false), active_workers_(0) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&ThreadPool::workerThread, this);
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::shutdown() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        stopped_ = true;
    }
    condition_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::shutdownWait() {
    shutdown();
}

void ThreadPool::pause() {
    std::unique_lock<std::mutex> lock(mutex_);
    paused_ = true;
}

void ThreadPool::resume() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        paused_ = false;
    }
    condition_.notify_all();
}

size_t ThreadPool::numPendingTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

void ThreadPool::workerThread() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(mutex_);

            condition_.wait(lock, [this] {
                return stopped_ || !tasks_.empty();
            });

            if (stopped_ && tasks_.empty()) {
                return;
            }

            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }

        if (task) {
            ++active_workers_;
            task();
            --active_workers_;
        }
    }
}

ThreadPool& globalThreadPool() {
    static ThreadPool pool;
    return pool;
}

} // namespace framework
} // namespace tradex