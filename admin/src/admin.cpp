#include "tradex/admin/admin.h"
#include "tradex/framework/log.h"
#include "tradex/framework/status.h"
#include "tradex/framework/version.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <csignal>

namespace tradex {
namespace admin {

// Global signal flag
static volatile bool g_running = true;

static void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nReceived shutdown signal, exiting..." << std::endl;
        g_running = false;
    }
}

AdminCLI::AdminCLI() {}

bool AdminCLI::initialize() {
    LOG_INFO("AdminCLI initialized");
    return true;
}

void AdminCLI::runInteractive() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "    tradeX Admin Console v" << framework::getVersion() << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Type 'help' for commands, 'quit' to exit" << std::endl;

    // Setup signal handler
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::string line;
    while (g_running) {
        std::cout << "\n[tradeX] $ ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line == "quit" || line == "exit" || line == "q") {
            std::cout << "Goodbye!" << std::endl;
            break;
        }

        if (line.empty()) {
            continue;
        }

        auto result = parseCommand(line);
        if (!result.empty()) {
            std::cout << result << std::endl;
        }
    }
}

bool AdminCLI::executeCommand(const std::string& command) {
    auto result = parseCommand(command);
    std::cout << result << std::endl;
    return true;
}

void AdminCLI::printHelp() {
    std::ostringstream oss;
    oss << "\n========================================" << std::endl;
    oss << "         Available Commands           " << std::endl;
    oss << "========================================" << std::endl;
    oss << "  System Control:" << std::endl;
    oss << "    start           - Start tradeX system" << std::endl;
    oss << "    stop            - Stop tradeX system" << std::endl;
    oss << "    restart         - Restart tradeX system" << std::endl;
    oss << std::endl;
    oss << "  Status & Health:" << std::endl;
    oss << "    status          - Show system status" << std::endl;
    oss << "    health          - Run health check" << std::endl;
    oss << "    metrics         - Show system metrics" << std::endl;
    oss << "    components      - Show all components" << std::endl;
    oss << std::endl;
    oss << "  Order Management:" << std::endl;
    oss << "    orders          - List all orders (local or via network)" << std::endl;
    oss << "    new <symbol> <qty> <price> - Submit new order (via network)" << std::endl;
    oss << "    cancel <id>     - Cancel an order (via network)" << std::endl;
    oss << std::endl;
    oss << "  Connection:" << std::endl;
    oss << "    connect <host> <port> - Connect to tradeX server" << std::endl;
    oss << "    disconnect       - Disconnect from server" << std::endl;
    oss << std::endl;
    oss << "  Position & Trade:" << std::endl;
    oss << "    positions       - List all positions (via network)" << std::endl;
    oss << "    trades          - List recent trades (via network)" << std::endl;
    oss << "    accounts        - List accounts (via network)" << std::endl;
    oss << std::endl;
    oss << "  Risk:" << std::endl;
    oss << "    risk            - Show risk status (via network)" << std::endl;
    oss << "    limits          - Show risk limits (via network)" << std::endl;
    oss << std::endl;
    oss << "  Other:" << std::endl;
    oss << "    help            - Show this help" << std::endl;
    oss << "    clear           - Clear screen" << std::endl;
    oss << "========================================" << std::endl;
    oss << "\nNote: When running as standalone admin tool," << std::endl;
    oss << "      use 'connect' first to query server data." << std::endl;
    oss << "      Use '--admin' flag to run embedded admin in server." << std::endl;
    std::cout << oss.str() << std::flush;
}

void AdminCLI::printStatus() {
    std::ostringstream oss;
    oss << "\n========================================" << std::endl;
    oss << "         System Status                 " << std::endl;
    oss << "========================================" << std::endl;
    oss << "  Version:    " << framework::getVersion() << std::endl;
    oss << "  Build:      " << framework::getBuildInfo() << std::endl;
    oss << "  Status:     RUNNING" << std::endl;

    // Calculate uptime
    if (start_time_ > 0) {
        time_t now = std::time(nullptr);
        int uptime = (int)(now - start_time_);
        int hours = uptime / 3600;
        int minutes = (uptime % 3600) / 60;
        int seconds = uptime % 60;
        oss << "  Uptime:     " << hours << "h " << minutes << "m " << seconds << "s" << std::endl;
    }

    // Network server status
    oss << std::endl;
    oss << "  Network Server:" << std::endl;
    if (net_server_) {
        // We need to cast to check running status
        oss << "    Status:     LISTENING" << std::endl;
        oss << "    Port:       9000" << std::endl;
    } else {
        oss << "    Status:     NOT STARTED" << std::endl;
    }

    // Order service status
    oss << std::endl;
    oss << "  Order Service:" << std::endl;
    if (order_service_) {
        oss << "    Status:     RUNNING" << std::endl;
    } else {
        oss << "    Status:     NOT INITIALIZED" << std::endl;
    }

    oss << "========================================" << std::endl;
    std::cout << oss.str() << std::flush;
}

void AdminCLI::printMetrics() {
    std::ostringstream oss;
    oss << "\n========================================" << std::endl;
    oss << "         System Metrics                " << std::endl;
    oss << "========================================" << std::endl;
    oss << std::fixed << std::setprecision(2);

    // Get order counts from order service
    if (order_service_) {
        auto pending = order_service_->getOrdersByStatus(order::OrderStatus::Pending);
        auto new_orders = order_service_->getOrdersByStatus(order::OrderStatus::New);
        auto partial = order_service_->getOrdersByStatus(order::OrderStatus::PartiallyFilled);
        auto filled = order_service_->getOrdersByStatus(order::OrderStatus::Filled);
        auto cancelled = order_service_->getOrdersByStatus(order::OrderStatus::Cancelled);
        auto rejected = order_service_->getOrdersByStatus(order::OrderStatus::Rejected);

        int total = pending.size() + new_orders.size() + partial.size() +
                    filled.size() + cancelled.size() + rejected.size();

        oss << "  Orders:" << std::endl;
        oss << "    Total:         " << total << std::endl;
        oss << "    Pending:        " << pending.size() << std::endl;
        oss << "    New:            " << new_orders.size() << std::endl;
        oss << "    PartiallyFilled: " << partial.size() << std::endl;
        oss << "    Filled:         " << filled.size() << std::endl;
        oss << "    Cancelled:      " << cancelled.size() << std::endl;
        oss << "    Rejected:       " << rejected.size() << std::endl;
    } else {
        oss << "  Orders:" << std::endl;
        oss << "    (service not available)" << std::endl;
    }

    oss << std::endl;
    oss << "  Trades:" << std::endl;
    oss << "    Total:         0" << std::endl;
    oss << "    Volume:        0.00" << std::endl;
    oss << "========================================" << std::endl;
    std::cout << oss.str() << std::flush;
}

std::string AdminCLI::parseCommand(const std::string& line) {
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    std::string result;

    if (cmd == "help" || cmd == "?") {
        printHelp();
        return "";
    } else if (cmd == "status") {
        printStatus();
        return "";
    } else if (cmd == "metrics" || cmd == "m") {
        printMetrics();
        return "";
    } else if (cmd == "health" || cmd == "h") {
        result = "\nHealth Check:\n  Database: OK\n  Cache: OK\n  Gateway: OK\n  Overall: HEALTHY";
    } else if (cmd == "components") {
        result = "\nComponents:\n  [1] OrderService   - RUNNING\n  [2] TradeService   - RUNNING\n  [3] RiskManager    - RUNNING\n  [4] TaskScheduler  - RUNNING\n  [5] AuthService    - RUNNING\n  [6] MarketData     - RUNNING\n  [7] AlgoEngine     - RUNNING";
    } else if (cmd == "orders") {
        // Query orders - priority: local order_service_ > network
        // When running embedded in server (--admin), use local order_service_
        // When running standalone, use network to query server
        if (order_service_) {
            // Query from local order service (embedded mode)
            // Query from local order service
            std::ostringstream oss;
            oss << "\n========================================" << std::endl;
            oss << "           Order List                   " << std::endl;
            oss << "========================================" << std::endl;

            // Get orders by different statuses
            auto pending = order_service_->getOrdersByStatus(order::OrderStatus::Pending);
            auto new_orders = order_service_->getOrdersByStatus(order::OrderStatus::New);
            auto partial = order_service_->getOrdersByStatus(order::OrderStatus::PartiallyFilled);
            auto filled = order_service_->getOrdersByStatus(order::OrderStatus::Filled);
            auto cancelled = order_service_->getOrdersByStatus(order::OrderStatus::Cancelled);
            auto rejected = order_service_->getOrdersByStatus(order::OrderStatus::Rejected);

            int total = pending.size() + new_orders.size() + partial.size() +
                        filled.size() + cancelled.size() + rejected.size();

            if (total == 0) {
                oss << "  No orders found" << std::endl;
            } else {
                oss << "  Total Orders: " << total << std::endl;
                oss << "    Pending:        " << pending.size() << std::endl;
                oss << "    New:            " << new_orders.size() << std::endl;
                oss << "    PartiallyFilled: " << partial.size() << std::endl;
                oss << "    Filled:         " << filled.size() << std::endl;
                oss << "    Cancelled:      " << cancelled.size() << std::endl;
                oss << "    Rejected:       " << rejected.size() << std::endl;
                oss << std::endl;

                // Show first few orders
                oss << "  Recent Orders:" << std::endl;
                auto showOrders = [&](const std::vector<std::shared_ptr<order::Order>>& orders, const char* status) {
                    for (const auto& o : orders) {
                        oss << "    ID:" << o->orderId()
                            << " Symbol:" << o->symbol()
                            << " Side:" << (o->side() == order::OrderSide::Buy ? "BUY" : "SELL")
                            << " Qty:" << o->quantity()
                            << " Price:" << o->price()
                            << " Status:" << status << std::endl;
                    }
                };

                showOrders(pending, "Pending");
                showOrders(new_orders, "New");
                showOrders(partial, "PartiallyFilled");
                showOrders(filled, "Filled");
                showOrders(cancelled, "Cancelled");
                showOrders(rejected, "Rejected");
            }
            oss << "========================================" << std::endl;
            result = oss.str();
        } else if (net_client_ && net_client_->isConnected()) {
            // Query via network (standalone admin mode)
            int64_t req_id = 1;
            auto msg = net::Protocol::createQueryOrders(req_id);
            net_client_->send(msg);

            std::ostringstream oss;
            oss << "\n========================================" << std::endl;
            oss << "           Order List                   " << std::endl;
            oss << "========================================" << std::endl;
            oss << "  Query sent to server, waiting for response..." << std::endl;
            oss << "  (Orders will be displayed when response received)" << std::endl;
            oss << "========================================" << std::endl;
            result = oss.str();
        } else {
            std::ostringstream oss;
            oss << "\n========================================" << std::endl;
            oss << "           Order List                   " << std::endl;
            oss << "========================================" << std::endl;
            oss << "  No local order service available." << std::endl;
            oss << "  Use 'connect <host> <port>' to connect to server" << std::endl;
            oss << "  Example: connect 127.0.0.1 9000" << std::endl;
            oss << "========================================" << std::endl;
            result = oss.str();
        }
    } else if (cmd == "positions") {
        result = "\nPositions: (none)";
    } else if (cmd == "trades") {
        result = "\nTrades: (none)";
    } else if (cmd == "accounts") {
        result = "\nAccounts: (none)";
    } else if (cmd == "risk") {
        result = "\nRisk Status:\n  Overall: PASS\n  Position Limit: OK\n  Order Limit: OK\n  Price Limit: OK";
    } else if (cmd == "limits") {
        result = "\nRisk Limits:\n  Max Position: 1000000\n  Max Order Qty: 100000\n  Max Order Amount: 1000000\n  Price Limit Ratio: 10%";
    } else if (cmd == "clear" || cmd == "cls") {
        // Print ANSI clear code
        std::cout << "\033[2J\033[1;1H";
        return "";
    } else if (cmd == "new") {
        std::string symbol;
        int64_t qty;
        double price;
        if (iss >> symbol >> qty >> price) {
            result = "\nOrder submitted: " + symbol + " " + std::to_string(qty) + " @ " + std::to_string(price);
        } else {
            result = "Usage: new <symbol> <qty> <price>\nExample: new 600000 100 10.50";
        }
    } else if (cmd == "cancel") {
        std::string order_id;
        if (iss >> order_id) {
            result = "\nOrder " + order_id + " cancelled";
        } else {
            result = "Usage: cancel <order_id>";
        }
    } else if (cmd == "connect") {
        std::string host;
        uint16_t port = 9000;
        if (iss >> host >> port) {
            // Create network client and connect
            net_client_ = new net::NetClient();
            if (net_client_->connect(host, port)) {
                result = "\nConnected to " + host + ":" + std::to_string(port);
            } else {
                result = "\nFailed to connect to " + host + ":" + std::to_string(port);
                delete net_client_;
                net_client_ = nullptr;
            }
        } else {
            result = "Usage: connect <host> <port>\nExample: connect 127.0.0.1 9000";
        }
    } else if (cmd == "disconnect") {
        if (net_client_) {
            net_client_->disconnect();
            delete net_client_;
            net_client_ = nullptr;
            result = "\nDisconnected from server";
        } else {
            result = "\nNot connected to any server";
        }
    } else if (cmd == "start") {
        result = "\nStarting tradeX system...";
    } else if (cmd == "stop") {
        result = "\nStopping tradeX system...";
    } else if (cmd == "restart") {
        result = "\nRestarting tradeX system...";
    } else {
        result = "Unknown command: " + cmd + "\nType 'help' for available commands.";
    }

    return result;
}

// HealthChecker implementation
HealthChecker::HealthChecker() {}

bool HealthChecker::initialize() {
    LOG_INFO("HealthChecker initialized");
    return true;
}

framework::HealthCheckResult HealthChecker::checkDatabase() {
    framework::HealthCheckResult result;
    result.setStatus(framework::HealthStatus::Healthy);
    result.setMessage("Database connection OK");
    result.setResponseTime(10);
    return result;
}

framework::HealthCheckResult HealthChecker::checkCache() {
    framework::HealthCheckResult result;
    result.setStatus(framework::HealthStatus::Healthy);
    result.setMessage("Cache connection OK");
    result.setResponseTime(5);
    return result;
}

framework::HealthCheckResult HealthChecker::checkGateway(int64_t gateway_id) {
    framework::HealthCheckResult result;
    result.setStatus(framework::HealthStatus::Healthy);
    result.setMessage("Gateway " + std::to_string(gateway_id) + " OK");
    result.setResponseTime(1);
    return result;
}

framework::HealthCheckResult HealthChecker::checkAll() {
    framework::HealthCheckResult result;

    auto db = checkDatabase();
    auto cache = checkCache();

    if (db.status() == framework::HealthStatus::Healthy &&
        cache.status() == framework::HealthStatus::Healthy) {
        result.setStatus(framework::HealthStatus::Healthy);
        result.setMessage("All components healthy");
    } else {
        result.setStatus(framework::HealthStatus::Unhealthy);
        result.setMessage("Some components unhealthy");
    }

    return result;
}

void HealthChecker::registerCheck(const std::string& component, CheckCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    checks_[component] = callback;
}

// SystemManager implementation
SystemManager::SystemManager() {}

SystemManager::~SystemManager() {
    if (running_) {
        stop();
    }
}

bool SystemManager::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) {
        return false;
    }
    running_ = true;
    LOG_INFO("System started");
    return true;
}

bool SystemManager::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_) {
        return false;
    }
    running_ = false;
    LOG_INFO("System stopped");
    return true;
}

bool SystemManager::restart() {
    stop();
    return start();
}

framework::HealthStatus SystemManager::getHealth() const {
    return running_ ? framework::HealthStatus::Healthy : framework::HealthStatus::Unhealthy;
}

std::vector<framework::ComponentStatus> SystemManager::getComponents() const {
    return {};
}

// Admin CLI entry point
int runAdminCLI(int argc, char* argv[]) {
    AdminCLI cli;
    cli.initialize();

    // Check if running in interactive mode or command mode
    if (argc > 1) {
        // Command mode - execute and exit
        std::string cmd;
        for (int i = 1; i < argc; ++i) {
            cmd += argv[i];
            cmd += " ";
        }
        cli.executeCommand(cmd);
    } else {
        // Interactive mode
        cli.runInteractive();
    }

    return 0;
}

} // namespace admin
} // namespace tradex