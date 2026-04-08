#include "tradex/framework/status.h"

namespace tradex {
namespace framework {

std::string Status::toString() const {
    std::string result;
    switch (code_) {
        case StatusCode::OK:
            result = "OK";
            break;
        case StatusCode::InvalidArgument:
            result = "INVALID_ARGUMENT";
            break;
        case StatusCode::NotFound:
            result = "NOT_FOUND";
            break;
        case StatusCode::AlreadyExists:
            result = "ALREADY_EXISTS";
            break;
        case StatusCode::PermissionDenied:
            result = "PERMISSION_DENIED";
            break;
        case StatusCode::ResourceExhausted:
            result = "RESOURCE_EXHAUSTED";
            break;
        case StatusCode::InternalError:
            result = "INTERNAL_ERROR";
            break;
        case StatusCode::Unavailable:
            result = "UNAVAILABLE";
            break;
        case StatusCode::Cancelled:
            result = "CANCELLED";
            break;
        case StatusCode::Timeout:
            result = "TIMEOUT";
            break;
        case StatusCode::DatabaseError:
            result = "DATABASE_ERROR";
            break;
        case StatusCode::NetworkError:
            result = "NETWORK_ERROR";
            break;
        case StatusCode::RiskRejected:
            result = "RISK_REJECTED";
            break;
        case StatusCode::OrderRejected:
            result = "ORDER_REJECTED";
            break;
        case StatusCode::InsufficientFunds:
            result = "INSUFFICIENT_FUNDS";
            break;
        case StatusCode::PositionLimitExceeded:
            result = "POSITION_LIMIT_EXCEEDED";
            break;
        default:
            result = "UNKNOWN";
            break;
    }

    if (!message_.empty()) {
        result += ": " + message_;
    }

    return result;
}

// ComponentStatus implementation
ComponentStatus::ComponentStatus(const std::string& name)
    : name_(name), total_requests_(0), failed_requests_(0) {}

void ComponentStatus::update(const HealthCheckResult& result) {
    last_result_ = result;
    last_update_ = std::chrono::steady_clock::now();
}

HealthCheckResult ComponentStatus::lastResult() const {
    return last_result_;
}

double ComponentStatus::errorRate() const {
    int64_t total = total_requests_.load();
    if (total == 0) return 0.0;
    return static_cast<double>(failed_requests_.load()) / static_cast<double>(total);
}

void ComponentStatus::recordRequest(bool success) {
    total_requests_++;
    if (!success) {
        failed_requests_++;
    }
}

// SystemStatus implementation
SystemStatus& SystemStatus::instance() {
    static SystemStatus status;
    return status;
}

void SystemStatus::registerComponent(const std::string& name,
                                      const HealthCheckResult& initial_result) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& comp : components_) {
        if (comp->name() == name) {
            comp->update(initial_result);
            return;
        }
    }

    auto comp = std::make_unique<ComponentStatus>(name);
    comp->update(initial_result);
    components_.push_back(std::move(comp));
}

void SystemStatus::updateComponent(const std::string& name,
                                    const HealthCheckResult& result) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& comp : components_) {
        if (comp->name() == name) {
            comp->update(result);
            return;
        }
    }
}

HealthStatus SystemStatus::getSystemHealth() const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (components_.empty()) {
        return HealthStatus::Unknown;
    }

    bool has_unhealthy = false;
    bool has_degraded = false;

    for (const auto& comp : components_) {
        switch (comp->lastResult().status()) {
            case HealthStatus::Unhealthy:
                has_unhealthy = true;
                break;
            case HealthStatus::Degraded:
                has_degraded = true;
                break;
            default:
                break;
        }
    }

    if (has_unhealthy) {
        return HealthStatus::Unhealthy;
    }
    if (has_degraded) {
        return HealthStatus::Degraded;
    }

    return HealthStatus::Healthy;
}

std::vector<std::reference_wrapper<const ComponentStatus>> SystemStatus::getAllComponents() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::reference_wrapper<const ComponentStatus>> result;
    for (const auto& comp : components_) {
        result.push_back(std::cref(*comp));
    }
    return result;
}

HealthCheckResult SystemStatus::checkHealth() {
    HealthCheckResult result;
    result.setStatus(getSystemHealth());

    std::lock_guard<std::mutex> lock(mutex_);
    int healthy_count = 0;
    int total_count = components_.size();

    for (const auto& comp : components_) {
        if (comp->lastResult().status() == HealthStatus::Healthy) {
            healthy_count++;
        }
    }

    std::ostringstream oss;
    oss << "System health: " << healthy_count << "/" << total_count << " components healthy";
    result.setMessage(oss.str());

    return result;
}

} // namespace framework
} // namespace tradex