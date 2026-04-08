#include "tradex/task/task.h"
#include "tradex/framework/log.h"
#include <chrono>

namespace tradex {
namespace task {

Task::Task(const std::string& name, TaskFunction func, TaskPriority priority)
    : name_(name), func_(func), priority_(priority),
      status_(TaskStatus::Pending), success_(false),
      create_time_(std::time(nullptr)), start_time_(0), end_time_(0) {}

bool Task::execute() {
    status_ = TaskStatus::Running;
    start_time_ = std::time(nullptr);

    try {
        success_ = func_();
        status_ = success_ ? TaskStatus::Completed : TaskStatus::Failed;
    } catch (const std::exception& e) {
        error_ = e.what();
        status_ = TaskStatus::Failed;
        success_ = false;
    }

    end_time_ = std::time(nullptr);
    return success_;
}

// TaskQueue implementation
TaskQueue::TaskQueue(size_t max_size) : max_size_(max_size), shutdown_(false) {}

TaskQueue::~TaskQueue() {
    shutdown_ = true;
    cv_.notify_all();
}

bool TaskQueue::push(std::shared_ptr<Task> task) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (queue_.size() >= max_size_) {
        return false;
    }

    queue_.push(task);
    cv_.notify_one();
    return true;
}

std::shared_ptr<Task> TaskQueue::pop() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (queue_.empty()) {
        return nullptr;
    }

    auto task = queue_.top();
    queue_.pop();
    return task;
}

std::shared_ptr<Task> TaskQueue::popWithTimeout(int64_t timeout_ms) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                     [this] { return !queue_.empty() || shutdown_; })) {
        if (shutdown_ || queue_.empty()) {
            return nullptr;
        }
        auto task = queue_.top();
        queue_.pop();
        return task;
    }

    return nullptr;
}

size_t TaskQueue::size() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    return queue_.size();
}

bool TaskQueue::empty() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    return queue_.empty();
}

void TaskQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    // Reconstruct with empty queue
    while (!queue_.empty()) {
        queue_.pop();
    }
}

// TaskScheduler implementation
TaskScheduler::TaskScheduler()
    : running_(false), next_task_id_(1) {}

TaskScheduler::~TaskScheduler() {
    shutdown();
}

bool TaskScheduler::initialize(size_t num_threads) {
    queue_ = std::make_unique<TaskQueue>();
    running_ = true;

    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&TaskScheduler::workerLoop, this);
    }

    LOG_INFO("TaskScheduler initialized with " + std::to_string(num_threads) + " threads");
    return true;
}

int64_t TaskScheduler::submit(const std::string& name,
                                Task::TaskFunction func,
                                TaskPriority priority) {
    auto task = std::make_shared<Task>(name, func, priority);
    int64_t task_id = next_task_id_++;

    queue_->push(task);
    return task_id;
}

int64_t TaskScheduler::submitDelayed(const std::string& name,
                                      Task::TaskFunction func,
                                      int64_t delay_ms,
                                      TaskPriority priority) {
    auto wrapped_func = [func, delay_ms]() -> bool {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        return func();
    };
    return submit(name, wrapped_func, priority);
}

int64_t TaskScheduler::submitPeriodic(const std::string& name,
                                       Task::TaskFunction func,
                                       int64_t interval_ms,
                                       TaskPriority priority) {
    auto periodic_func = [this, name, func, interval_ms, priority]() -> bool {
        while (running_) {
            func();
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
        }
        return true;
    };
    return submit(name, periodic_func, priority);
}

bool TaskScheduler::cancel(int64_t task_id) {
    // Simplified - in real implementation would track tasks
    return false;
}

TaskStatus TaskScheduler::getTaskStatus(int64_t task_id) const {
    return TaskStatus::Pending;
}

void TaskScheduler::shutdown() {
    running_ = false;
}

void TaskScheduler::shutdownWait() {
    shutdown();
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

size_t TaskScheduler::pendingTasks() const {
    if (queue_) {
        return queue_->size();
    }
    return 0;
}

size_t TaskScheduler::activeTasks() const {
    return workers_.size();
}

void TaskScheduler::workerLoop() {
    while (running_) {
        auto task = queue_->popWithTimeout(100);
        if (task) {
            task->execute();
        }
    }
}

} // namespace task
} // namespace tradex