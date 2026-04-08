#pragma once

#include "tradex/order/order.h"
#include "tradex/trade/trade.h"
#include "tradex/framework/status.h"
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <mutex>

namespace tradex {
namespace gateway {

// Exchange type
enum class Exchange {
    SSE = 1,    // Shanghai Stock Exchange
    SZSE = 2,   // Shenzhen Stock Exchange
    BSE = 3     // Beijing Stock Exchange
};

// Gateway status
enum class GatewayStatus {
    Disconnected = 0,
    Connecting = 1,
    Connected = 2,
    Authenticated = 3,
    Error = 4
};

// Order route info
struct OrderRoute {
    int64_t order_id;
    int64_t gateway_id;
    std::string exchange_order_id;
    Exchange exchange;
    int64_t send_time;
    int64_t recv_time;
};

// Gateway interface
class IGateway {
public:
    virtual ~IGateway() = default;

    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual GatewayStatus status() const = 0;

    virtual framework::Status sendOrder(std::shared_ptr<order::Order> order) = 0;
    virtual framework::Status cancelOrder(int64_t order_id) = 0;
    virtual framework::Status modifyOrder(int64_t order_id, int64_t new_qty, double new_price) = 0;

    virtual std::string name() const = 0;
};

// Order router
class OrderRouter {
public:
    OrderRouter();
    ~OrderRouter() = default;

    bool initialize();

    // Register gateway
    void registerGateway(int64_t gateway_id, std::shared_ptr<IGateway> gateway);
    void unregisterGateway(int64_t gateway_id);

    // Route order to gateway
    framework::Status routeOrder(std::shared_ptr<order::Order> order);
    framework::Status routeCancel(int64_t order_id);
    framework::Status routeModify(int64_t order_id, int64_t new_qty, double new_price);

    // Gateway management
    std::shared_ptr<IGateway> getGateway(int64_t gateway_id);
    std::vector<std::shared_ptr<IGateway>> getAllGateways();

    // Statistics
    size_t activeGateways() const;
    size_t totalOrdersSent() const { return total_orders_sent_; }

    // Set order update callback
    using OrderUpdateCallback = std::function<void(const order::OrderUpdate&)>;
    void setOrderUpdateCallback(OrderUpdateCallback cb) { order_update_cb_ = cb; }

private:
    std::shared_ptr<IGateway> selectGateway(const std::string& symbol);

    mutable std::mutex mutex_;
    std::unordered_map<int64_t, std::shared_ptr<IGateway>> gateways_;

    size_t total_orders_sent_;
    OrderUpdateCallback order_update_cb_;
};

// Simulated exchange gateway
class SimExchangeGateway : public IGateway {
public:
    explicit SimExchangeGateway(const std::string& name);
    ~SimExchangeGateway() override;

    bool connect() override;
    void disconnect() override;
    GatewayStatus status() const override;

    framework::Status sendOrder(std::shared_ptr<order::Order> order) override;
    framework::Status cancelOrder(int64_t order_id) override;
    framework::Status modifyOrder(int64_t order_id, int64_t new_qty, double new_price) override;

    std::string name() const override { return name_; }

    void setExchange(Exchange exchange) { exchange_ = exchange; }
    void setLatencyMs(int latency_ms) { latency_ms_ = latency_ms; }

private:
    void simulateFill(int64_t order_id);
    void simulateReject(int64_t order_id, const std::string& reason);

    std::string name_;
    Exchange exchange_;
    GatewayStatus status_;
    int latency_ms_;

    mutable std::mutex mutex_;
    std::unordered_map<int64_t, std::shared_ptr<order::Order>> pending_orders_;
};

} // namespace gateway
} // namespace tradex