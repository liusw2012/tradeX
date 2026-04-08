#pragma once

#include "tradex/framework/status.h"
#include "tradex/order/order_service.h"
#include "tradex/net/net.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <iostream>
#include <sstream>
#include <mutex>
#include <unordered_map>

namespace tradex {
namespace admin {

// Admin command types
enum class AdminCommand {
    SubmitOrder = 1,
    CancelOrder = 2,
    ModifyOrder = 3,
    QueryOrder = 4,
    QueryPosition = 5,
    QueryTrade = 6,
    QueryAccount = 7,
    Start = 8,
    Stop = 9,
    Restart = 10,
    Status = 11,
    Metrics = 12,
    HealthCheck = 13,
    Config = 14
};

// Command result
struct CommandResult {
    bool success;
    std::string message;
    std::string data;
};

// Callback types
using OutputCallback = std::function<void(const std::string&)>;
using CommandCallback = std::function<CommandResult(const std::vector<std::string>&)>;

// Admin CLI - main admin interface
class AdminCLI {
public:
    AdminCLI();
    ~AdminCLI() = default;

    bool initialize();
    void runInteractive();
    bool executeCommand(const std::string& command);

    // Output callback
    void setOutputCallback(OutputCallback cb) { output_cb_ = cb; }

    // Get service instances
    void setOrderService(order::IOrderService* service) { order_service_ = service; }
    void setTradeService(void* service) { trade_service_ = service; }
    void setRiskManager(void* service) { risk_manager_ = service; }
    void setNetworkServer(void* service) { net_server_ = service; }
    void setStartTime(time_t t) { start_time_ = t; }

    // Network client for admin mode
    void setNetworkClient(net::NetClient* client) { net_client_ = client; }
    bool connectToServer(const std::string& host, uint16_t port);

    // Get all orders (for admin queries)
    std::vector<std::shared_ptr<order::Order>> getAllOrders() const;

private:
    void printHelp();
    void printStatus();
    void printMetrics();
    std::string parseCommand(const std::string& line);

    OutputCallback output_cb_;
    order::IOrderService* order_service_ = nullptr;
    void* trade_service_ = nullptr;
    void* risk_manager_ = nullptr;
    void* net_server_ = nullptr;
    time_t start_time_ = 0;
    net::NetClient* net_client_ = nullptr;
};

// Health check types
enum class HealthCheckType {
    Database = 1,
    Cache = 2,
    Gateway = 3,
    All = 4
};

// Health checker
class HealthChecker {
public:
    HealthChecker();
    ~HealthChecker() = default;

    bool initialize();

    framework::HealthCheckResult checkDatabase();
    framework::HealthCheckResult checkCache();
    framework::HealthCheckResult checkGateway(int64_t gateway_id);
    framework::HealthCheckResult checkAll();

    using CheckCallback = std::function<framework::HealthCheckResult()>;
    void registerCheck(const std::string& component, CheckCallback callback);

private:
    std::mutex mutex_;
    std::unordered_map<std::string, CheckCallback> checks_;
};

// System manager - controls tradeX lifecycle
class SystemManager {
public:
    SystemManager();
    ~SystemManager();

    bool start();
    bool stop();
    bool restart();
    bool isRunning() const { return running_; }

    // Get component status
    framework::HealthStatus getHealth() const;
    std::vector<framework::ComponentStatus> getComponents() const;

private:
    bool running_ = false;
    std::mutex mutex_;
};

// CLI entry point
int runAdminCLI(int argc, char* argv[]);

} // namespace admin
} // namespace tradex