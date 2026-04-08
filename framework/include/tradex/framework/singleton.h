#pragma once

#include <mutex>
#include <memory>

namespace tradex {
namespace framework {

// Singleton template
template<typename T>
class Singleton {
public:
    static T& instance() {
        static T instance_;
        return instance_;
    }

protected:
    Singleton() = default;
    ~Singleton() = default;

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};

// Thread-safe singleton with double-checked locking
template<typename T>
class ThreadSafeSingleton {
public:
    static T* instance() {
        if (instance_ == nullptr) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (instance_ == nullptr) {
                instance_ = new T();
            }
        }
        return instance_;
    }

    static void destroy() {
        std::lock_guard<std::mutex> lock(mutex_);
        delete instance_;
        instance_ = nullptr;
    }

protected:
    ThreadSafeSingleton() = default;
    ~ThreadSafeSingleton() = default;

    ThreadSafeSingleton(const ThreadSafeSingleton&) = delete;
    ThreadSafeSingleton& operator=(const ThreadSafeSingleton&) = delete;

private:
    static T* instance_;
    static std::mutex mutex_;
};

template<typename T>
T* ThreadSafeSingleton<T>::instance_ = nullptr;

template<typename T>
std::mutex ThreadSafeSingleton<T>::mutex_;

// Singleton with lazy initialization and automatic cleanup
template<typename T>
class ManagedSingleton {
public:
    template<typename... Args>
    static T& get(Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!instance_) {
            instance_ = std::make_unique<T>(std::forward<Args>(args)...);
        }
        return *instance_;
    }

    static void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        instance_.reset();
    }

    static bool exists() {
        std::lock_guard<std::mutex> lock(mutex_);
        return instance_ != nullptr;
    }

protected:
    ManagedSingleton() = default;
    ~ManagedSingleton() = default;

    ManagedSingleton(const ManagedSingleton&) = delete;
    ManagedSingleton& operator=(const ManagedSingleton&) = delete;

private:
    static std::unique_ptr<T> instance_;
    static std::mutex mutex_;
};

template<typename T>
std::unique_ptr<T> ManagedSingleton<T>::instance_;

template<typename T>
std::mutex ManagedSingleton<T>::mutex_;

} // namespace framework
} // namespace tradex