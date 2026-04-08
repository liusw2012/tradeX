#pragma once

#include <string>
#include <functional>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <thread>

namespace tradex {
namespace task {

// Task priority
enum class TaskPriority {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

// Task status
enum class TaskStatus {
    Pending = 0,
    Running = 1,
    Completed = 2,
    Failed = 3,
    Cancelled = 4
};

// Task base
class Task {
public:
    using TaskFunction = std::function<bool()>;

    Task(const std::string& name, TaskFunction func, TaskPriority priority = TaskPriority::Normal);

    int64_t id() const { return id_; }
    const std::string& name() const { return name_; }
    TaskPriority priority() const { return priority_; }
    TaskStatus status() const { return status_; }

    void setStatus(TaskStatus status) { status_ = status; }
    void setResult(bool success) { success_ = success; }
    void setError(const std::string& error) { error_ = error; }

    bool execute();

private:
    int64_t id_;
    std::string name_;
    TaskFunction func_;
    TaskPriority priority_;
    TaskStatus status_;
    bool success_;
    std::string error_;
    int64_t create_time_;
    int64_t start_time_;
    int64_t end_time_;
};

// Task queue
class TaskQueue {
public:
    TaskQueue(size_t max_size = 10000);
    ~TaskQueue();

    bool push(std::shared_ptr<Task> task);
    std::shared_ptr<Task> pop();
    std::shared_ptr<Task> popWithTimeout(int64_t timeout_ms);

    size_t size() const;
    bool empty() const;
    void clear();

private:
    struct TaskComparator {
        bool operator()(const std::shared_ptr<Task>& a,
                        const std::shared_ptr<Task>& b) const {
            return a->priority() < b->priority();
        }
    };

    std::priority_queue<std::shared_ptr<Task>,
                       std::vector<std::shared_ptr<Task>>,
                       TaskComparator> queue_;

    size_t max_size_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> shutdown_;
};

// Task scheduler
class TaskScheduler {
public:
    TaskScheduler();
    ~TaskScheduler();

    bool initialize(size_t num_threads = 4);

    // Submit task
    int64_t submit(const std::string& name,
                   Task::TaskFunction func,
                   TaskPriority priority = TaskPriority::Normal);

    // Submit delayed task
    int64_t submitDelayed(const std::string& name,
                          Task::TaskFunction func,
                          int64_t delay_ms,
                          TaskPriority priority = TaskPriority::Normal);

    // Submit periodic task
    int64_t submitPeriodic(const std::string& name,
                           Task::TaskFunction func,
                           int64_t interval_ms,
                           TaskPriority priority = TaskPriority::Normal);

    // Cancel task
    bool cancel(int64_t task_id);

    // Get task status
    TaskStatus getTaskStatus(int64_t task_id) const;

    // Shutdown
    void shutdown();
    void shutdownWait();

    // Statistics
    size_t pendingTasks() const;
    size_t activeTasks() const;

private:
    void workerLoop();

    std::unique_ptr<TaskQueue> queue_;
    std::vector<std::thread> workers_;
    std::atomic<bool> running_;
    std::atomic<int64_t> next_task_id_;
};

} // namespace task
} // namespace tradex