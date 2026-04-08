#include "tradex/gateway/gateway.h"
#include "tradex/framework/log.h"
#include <thread>
#include <random>

namespace tradex {
namespace gateway {

OrderRouter::OrderRouter() : total_orders_sent_(0) {}

bool OrderRouter::initialize() {
    LOG_INFO("OrderRouter initialized");
    return true;
}

void OrderRouter::registerGateway(int64_t gateway_id, std::shared_ptr<IGateway> gateway) {
    std::lock_guard<std::mutex> lock(mutex_);
    gateways_[gateway_id] = gateway;
    LOG_INFO("Gateway registered: " + gateway->name());
}

void OrderRouter::unregisterGateway(int64_t gateway_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    gateways_.erase(gateway_id);
}

framework::Status OrderRouter::routeOrder(std::shared_ptr<order::Order> order) {
    auto gateway = selectGateway(order->symbol());
    if (!gateway) {
        return framework::Status::Unavailable("No gateway available for symbol: " + order->symbol());
    }

    auto status = gateway->sendOrder(order);
    if (status.ok()) {
        std::lock_guard<std::mutex> lock(mutex_);
        total_orders_sent_++;
    }

    return status;
}

framework::Status OrderRouter::routeCancel(int64_t order_id) {
    // In real implementation, would lookup gateway by order
    return framework::Status::OK();
}

framework::Status OrderRouter::routeModify(int64_t order_id, int64_t new_qty, double new_price) {
    return framework::Status::OK();
}

std::shared_ptr<IGateway> OrderRouter::getGateway(int64_t gateway_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = gateways_.find(gateway_id);
    if (it != gateways_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<IGateway>> OrderRouter::getAllGateways() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<IGateway>> result;
    for (const auto& kv : gateways_) {
        result.push_back(kv.second);
    }
    return result;
}

size_t OrderRouter::activeGateways() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    size_t count = 0;
    for (const auto& kv : gateways_) {
        if (kv.second->status() == GatewayStatus::Authenticated) {
            count++;
        }
    }
    return count;
}

std::shared_ptr<IGateway> OrderRouter::selectGateway(const std::string& symbol) {
    // Simple selection - pick first available gateway
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& kv : gateways_) {
        if (kv.second->status() == GatewayStatus::Authenticated) {
            return kv.second;
        }
    }
    return nullptr;
}

// SimExchangeGateway implementation
SimExchangeGateway::SimExchangeGateway(const std::string& name)
    : name_(name), exchange_(Exchange::SSE), status_(GatewayStatus::Disconnected),
      latency_ms_(10) {}

SimExchangeGateway::~SimExchangeGateway() {
    disconnect();
}

bool SimExchangeGateway::connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    status_ = GatewayStatus::Connecting;

    // Simulate connection delay
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    status_ = GatewayStatus::Authenticated;
    LOG_INFO("Gateway connected: " + name_);
    return true;
}

void SimExchangeGateway::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    status_ = GatewayStatus::Disconnected;
    pending_orders_.clear();
    LOG_INFO("Gateway disconnected: " + name_);
}

GatewayStatus SimExchangeGateway::status() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    return status_;
}

framework::Status SimExchangeGateway::sendOrder(std::shared_ptr<order::Order> order) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (status_ != GatewayStatus::Authenticated) {
        return framework::Status::Unavailable("Gateway not connected");
    }

    pending_orders_[order->orderId()] = order;

    // Simulate async fill
    std::thread([this, order]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(latency_ms_));
        simulateFill(order->orderId());
    }).detach();

    LOG_INFO("Order sent to gateway: " + name_ + ", order_id=" + std::to_string(order->orderId()));
    return framework::Status::OK();
}

framework::Status SimExchangeGateway::cancelOrder(int64_t order_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = pending_orders_.find(order_id);
    if (it == pending_orders_.end()) {
        return framework::Status::NotFound("Order not found");
    }

    pending_orders_.erase(it);
    LOG_INFO("Order cancelled in gateway: " + std::to_string(order_id));
    return framework::Status::OK();
}

framework::Status SimExchangeGateway::modifyOrder(int64_t order_id, int64_t new_qty, double new_price) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = pending_orders_.find(order_id);
    if (it == pending_orders_.end()) {
        return framework::Status::NotFound("Order not found");
    }

    it->second->setQuantity(new_qty);
    it->second->setPrice(new_price);
    LOG_INFO("Order modified in gateway: " + std::to_string(order_id));
    return framework::Status::OK();
}

void SimExchangeGateway::simulateFill(int64_t order_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = pending_orders_.find(order_id);
    if (it == pending_orders_.end()) {
        return;
    }

    auto order = it->second;

    // Simulate partial fill
    int64_t fill_qty = order->quantityRemaining();
    double fill_price = order->price();

    if (order->type() == order::OrderType::Market) {
        // Market order - use a random price
        fill_price = order->price() * 1.001;  // Slight slippage
    }

    LOG_INFO("Order filled: order_id=" + std::to_string(order_id) +
             ", qty=" + std::to_string(fill_qty) +
             ", price=" + std::to_string(fill_price));

    // Remove from pending
    pending_orders_.erase(it);
}

void SimExchangeGateway::simulateReject(int64_t order_id, const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_orders_.erase(order_id);
    LOG_WARNING("Order rejected: order_id=" + std::to_string(order_id) +
                 ", reason=" + reason);
}

} // namespace gateway
} // namespace tradex