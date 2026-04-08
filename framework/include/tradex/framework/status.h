#pragma once

#include <string>
#include <atomic>
#include <memory>
#include <vector>
#include <chrono>
#include <mutex>
#include <sstream>

namespace tradex {
namespace framework {

// Status codes
enum class StatusCode {
    OK = 0,
    InvalidArgument = 1,
    NotFound = 2,
    AlreadyExists = 3,
    PermissionDenied = 4,
    ResourceExhausted = 5,
    InternalError = 6,
    Unavailable = 7,
    Cancelled = 8,
    Timeout = 9,
    DatabaseError = 10,
    NetworkError = 11,
    RiskRejected = 12,
    OrderRejected = 13,
    InsufficientFunds = 14,
    PositionLimitExceeded = 15
};

// Status result
class Status {
public:
    Status() : code_(StatusCode::OK), message_() {}
    Status(StatusCode code, const std::string& message)
        : code_(code), message_(message) {}

    bool ok() const { return code_ == StatusCode::OK; }
    StatusCode code() const { return code_; }
    const std::string& message() const { return message_; }

    std::string toString() const;

    static Status OK() { return Status(); }
    static Status InvalidArgument(const std::string& msg) {
        return Status(StatusCode::InvalidArgument, msg);
    }
    static Status NotFound(const std::string& msg) {
        return Status(StatusCode::NotFound, msg);
    }
    static Status AlreadyExists(const std::string& msg) {
        return Status(StatusCode::AlreadyExists, msg);
    }
    static Status InternalError(const std::string& msg) {
        return Status(StatusCode::InternalError, msg);
    }
    static Status DatabaseError(const std::string& msg) {
        return Status(StatusCode::DatabaseError, msg);
    }
    static Status RiskRejected(const std::string& msg) {
        return Status(StatusCode::RiskRejected, msg);
    }
    static Status OrderRejected(const std::string& msg) {
        return Status(StatusCode::OrderRejected, msg);
    }
    static Status Unavailable(const std::string& msg) {
        return Status(StatusCode::Unavailable, msg);
    }

private:
    StatusCode code_;
    std::string message_;
};

// Component health status
enum class HealthStatus {
    Healthy = 0,
    Degraded = 1,
    Unhealthy = 2,
    Unknown = 3
};

// Health check result
class HealthCheckResult {
public:
    HealthCheckResult()
        : status_(HealthStatus::Unknown), response_time_ms_(0) {}

    HealthStatus status() const { return status_; }
    const std::string& message() const { return message_; }
    int64_t responseTimeMs() const { return response_time_ms_; }

    void setStatus(HealthStatus status) { status_ = status; }
    void setMessage(const std::string& msg) { message_ = msg; }
    void setResponseTime(int64_t ms) { response_time_ms_ = ms; }

private:
    HealthStatus status_;
    std::string message_;
    int64_t response_time_ms_;
};

// Component status for monitoring
class ComponentStatus {
public:
    ComponentStatus(const std::string& name);

    // Prevent copying (atomic cannot be copied)
    ComponentStatus(const ComponentStatus&) = delete;
    ComponentStatus& operator=(const ComponentStatus&) = delete;

    // Allow moving
    ComponentStatus(ComponentStatus&& other) noexcept;
    ComponentStatus& operator=(ComponentStatus&& other) noexcept;

    void update(const HealthCheckResult& result);
    HealthCheckResult lastResult() const;

    const std::string& name() const { return name_; }
    std::chrono::steady_clock::time_point lastUpdate() const { return last_update_; }

    // Metrics
    int64_t totalRequests() const { return total_requests_.load(); }
    int64_t failedRequests() const { return failed_requests_.load(); }
    double errorRate() const;

    void recordRequest(bool success);

private:
    std::string name_;
    HealthCheckResult last_result_;
    std::chrono::steady_clock::time_point last_update_;

    std::atomic<int64_t> total_requests_;
    std::atomic<int64_t> failed_requests_;
};

// System status manager
class SystemStatus {
public:
    static SystemStatus& instance();

    void registerComponent(const std::string& name,
                           const HealthCheckResult& initial_result);
    void updateComponent(const std::string& name,
                         const HealthCheckResult& result);

    HealthStatus getSystemHealth() const;
    std::vector<std::reference_wrapper<const ComponentStatus>> getAllComponents() const;

    // Global health check
    HealthCheckResult checkHealth();

private:
    SystemStatus() = default;
    ~SystemStatus() = default;

    SystemStatus(const SystemStatus&) = delete;
    SystemStatus& operator=(const SystemStatus&) = delete;

    mutable std::mutex mutex_;
    std::vector<std::unique_ptr<ComponentStatus>> components_;
};

} // namespace framework
} // namespace tradex