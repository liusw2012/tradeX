#include <iostream>
#include <memory>
#include <csignal>
#include <cstdlib>

// C++17 filesystem for cross-platform directory creation
#if __cplusplus >= 201703L
#include <filesystem>
#endif

#include "tradex/framework/log.h"
#include "tradex/framework/config.h"
#include "tradex/framework/version.h"
#include "tradex/framework/status.h"
#include "tradex/framework/thread_pool.h"

#include "tradex/order/order_service.h"
#include "tradex/trade/trade.h"
#include "tradex/risk/risk.h"
#include "tradex/task/task.h"
#include "tradex/auth/auth.h"
#include "tradex/gateway/gateway.h"
#include "tradex/market/market.h"
#include "tradex/algo/algo.h"
#include "tradex/strategy/strategy.h"
#include "tradex/admin/admin.h"
#include "tradex/net/net.h"

using namespace tradex;

// Global flag for signal handling
static bool g_running = true;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        LOG_INFO("Received shutdown signal, stopping...");
        g_running = false;
    }
}

class TradeXApp {
public:
    TradeXApp() = default;
    ~TradeXApp() = default;

    bool initialize() {
        // Print build info
        std::cout << framework::getBuildInfo() << std::endl;

        // Create logs directory using C++17 filesystem or fallback
#if __cplusplus >= 201703L
        std::filesystem::create_directories("logs");
#else
        // Fallback for older C++ standards - rely on CMake to create directory
#endif

        // Initialize logging
        framework::Logger::instance().setLevel(framework::LogLevel::Info);
        if (framework::Logger::instance().enableFileLog("logs/tradex.log")) {
            framework::Logger::instance().setMaxFileSize(10 * 1024 * 1024); // 10MB
            std::cout << "[LOG] File logging enabled: logs/tradex.log" << std::endl;
        } else {
            std::cout << "[LOG] Warning: Failed to enable file logging, using console only" << std::endl;
        }
        LOG_INFO("Initializing tradeX...");
        std::cout << "[SERVER] Initializing tradeX..." << std::endl;

        // Start network server
        uint16_t port = 9000;
        net_server_ = std::make_unique<net::NetServer>();
        net_server_->setMessageCallback([this](int64_t client_id, const net::Message& msg) {
            this->handleClientMessage(client_id, msg);
        });
        if (net_server_->start(port)) {
            LOG_INFO("Network server started on port " + std::to_string(port));
        } else {
            LOG_WARNING("Failed to start network server on port " + std::to_string(port));
        }

        // Initialize framework components
        if (!framework::Config::instance().loadFromFile("config/tradex.json")) {
            LOG_WARNING("Config file not found, using defaults");
        }

        // Initialize services
        order_service_ = std::make_unique<order::OrderService>();
        if (!order_service_->initialize()) {
            LOG_ERROR("Failed to initialize OrderService");
            return false;
        }

        trade_service_ = std::make_unique<trade::TradeService>();
        if (!trade_service_->initialize()) {
            LOG_ERROR("Failed to initialize TradeService");
            return false;
        }

        risk_manager_ = std::make_unique<risk::RiskManager>();
        if (!risk_manager_->initialize()) {
            LOG_ERROR("Failed to initialize RiskManager");
            return false;
        }

        task_scheduler_ = std::make_unique<task::TaskScheduler>();
        if (!task_scheduler_->initialize(4)) {
            LOG_ERROR("Failed to initialize TaskScheduler");
            return false;
        }

        auth_service_ = std::make_unique<auth::AuthService>();
        if (!auth_service_->initialize()) {
            LOG_ERROR("Failed to initialize AuthService");
            return false;
        }

        order_router_ = std::make_unique<gateway::OrderRouter>();
        if (!order_router_->initialize()) {
            LOG_ERROR("Failed to initialize OrderRouter");
            return false;
        }

        market_data_ = std::make_unique<market::MarketDataService>();
        if (!market_data_->initialize()) {
            LOG_ERROR("Failed to initialize MarketDataService");
            return false;
        }

        algo_engine_ = std::make_unique<algo::AlgoEngine>();
        if (!algo_engine_->initialize()) {
            LOG_ERROR("Failed to initialize AlgoEngine");
            return false;
        }

        admin_cli_ = std::make_unique<admin::AdminCLI>();
        if (!admin_cli_->initialize()) {
            LOG_ERROR("Failed to initialize AdminCLI");
            return false;
        }
        // Pass order service to admin CLI for querying orders
        admin_cli_->setOrderService(order_service_.get());
        admin_cli_->setNetworkServer(net_server_.get());
        admin_cli_->setStartTime(std::time(nullptr));

        // Register health checks
        framework::SystemStatus::instance().registerComponent("OrderService",
            framework::HealthCheckResult());
        framework::SystemStatus::instance().registerComponent("TradeService",
            framework::HealthCheckResult());
        framework::SystemStatus::instance().registerComponent("RiskManager",
            framework::HealthCheckResult());

        LOG_INFO("tradeX initialized successfully");
        return true;
    }

    void run() {
        LOG_INFO("Starting tradeX...");

        // Register signal handlers
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        // Main loop - in production would run server
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            // Periodic health check
            auto health = framework::SystemStatus::instance().checkHealth();
        }

        shutdown();
    }

    void runAdmin() {
        LOG_INFO("Starting admin console...");
        if (admin_cli_) {
            admin_cli_->runInteractive();
        }
        g_running = false;
    }

    void shutdown() {
        LOG_INFO("Shutting down tradeX...");

        if (net_server_) {
            net_server_->stop();
        }

        if (task_scheduler_) {
            task_scheduler_->shutdownWait();
        }

        LOG_INFO("tradeX shutdown complete");
    }

    // Network message handler
    void handleClientMessage(int64_t client_id, const net::Message& msg);

    // Process order from network
    net::Message processNetworkOrder(const std::string& body);

    // Process cancel from network
    net::Message processNetworkCancel(const std::string& body);

    // Process query from network
    net::Message processNetworkQuery(const std::string& body);

    // Demo: Submit a test order
    void submitTestOrder() {
        auto order = std::make_shared<order::Order>(
            "test_order_1",          // ClOrdId
            12345,                   // Account ID
            "600000.SS",             // Symbol (SSE)
            order::OrderSide::Buy,
            order::OrderType::Limit,
            1000                     // Quantity
        );
        order->setPrice(10.5);

        // Check risk
        auto risk_results = risk_manager_->checkOrder(*order);
        bool passed = true;
        for (const auto& result : risk_results) {
            if (!result.passed) {
                LOG_WARNING("Risk check failed: " + result.message);
                passed = false;
            }
        }

        if (passed) {
            auto status = order_service_->submitOrder(order);
            if (status.ok()) {
                LOG_INFO("Test order submitted successfully");
            } else {
                LOG_ERROR("Failed to submit order: " + status.toString());
            }
        }
    }

private:
    std::unique_ptr<order::OrderService> order_service_;
    std::unique_ptr<trade::TradeService> trade_service_;
    std::unique_ptr<risk::RiskManager> risk_manager_;
    std::unique_ptr<task::TaskScheduler> task_scheduler_;
    std::unique_ptr<auth::AuthService> auth_service_;
    std::unique_ptr<gateway::OrderRouter> order_router_;
    std::unique_ptr<market::MarketDataService> market_data_;
    std::unique_ptr<algo::AlgoEngine> algo_engine_;
    std::unique_ptr<admin::AdminCLI> admin_cli_;
    std::unique_ptr<net::NetServer> net_server_;
};

// Network message handler implementation
void TradeXApp::handleClientMessage(int64_t client_id, const net::Message& msg) {
    net::MsgType type = static_cast<net::MsgType>(msg.header.type);
    net::Message response;

    std::string typeName;
    switch (type) {
        case net::MsgType::SubmitOrder: typeName = "SubmitOrder"; break;
        case net::MsgType::CancelOrder: typeName = "CancelOrder"; break;
        case net::MsgType::QueryOrders: typeName = "QueryOrders"; break;
        case net::MsgType::QueryPositions: typeName = "QueryPositions"; break;
        case net::MsgType::Ping: typeName = "Ping"; break;
        default: typeName = "Unknown"; break;
    }

    std::cout << "[SERVER] Client " << client_id << " request: " << typeName
              << ", body: " << msg.body << std::endl;
    LOG_INFO("Processing request from client " + std::to_string(client_id) +
             ": type=" + typeName + ", body=" + msg.body);

    switch (type) {
        case net::MsgType::SubmitOrder:
            response = processNetworkOrder(msg.body);
            break;
        case net::MsgType::CancelOrder:
            response = processNetworkCancel(msg.body);
            break;
        case net::MsgType::QueryOrders:
        case net::MsgType::QueryPositions:
            std::cout << "[SERVER] Processing query orders request" << std::endl;
            LOG_INFO("Processing query orders request");
            response = processNetworkQuery(msg.body);
            break;
        case net::MsgType::Ping:
            response.header.magic = net::MsgHeader::MAGIC;
            response.header.type = static_cast<uint16_t>(net::MsgType::Pong);
            response.body = "{\"success\":true}";
            response.header.length = response.body.size();
            std::cout << "[SERVER] Responding to Ping" << std::endl;
            break;
        default:
            response.header.magic = net::MsgHeader::MAGIC;
            response.header.type = static_cast<uint16_t>(net::MsgType::ErrorResponse);
            response.body = "{\"success\":false,\"message\":\"Unknown request\"}";
            response.header.length = response.body.size();
            break;
    }

    if (net_server_) {
        net_server_->sendTo(client_id, response);
        std::cout << "[SERVER] Response sent to client " << client_id << std::endl;
        LOG_INFO("Response sent to client " + std::to_string(client_id));
    }
}

// Helper function to extract string value from JSON
std::string extractJsonString(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\":\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return "";
    pos += pattern.size();
    size_t end = json.find("\"", pos);
    if (end == std::string::npos) return "";
    return json.substr(pos, end - pos);
}

// Helper function to extract numeric value from JSON
int64_t extractJsonInt(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\":";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return 0;
    pos += pattern.size();
    size_t end = pos;
    while (end < json.size() && (isdigit(json[end]) || json[end] == '-')) {
        end++;
    }
    if (end == pos) return 0;
    return std::stoll(json.substr(pos, end - pos));
}

// Helper function to extract double value from JSON
double extractJsonDouble(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\":";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return 0.0;
    pos += pattern.size();
    size_t end = pos;
    while (end < json.size() && (isdigit(json[end]) || json[end] == '.' || json[end] == '-')) {
        end++;
    }
    if (end == pos) return 0.0;
    return std::stod(json.substr(pos, end - pos));
}

net::Message TradeXApp::processNetworkOrder(const std::string& body) {
    net::Message response;
    response.header.magic = net::MsgHeader::MAGIC;
    response.header.type = static_cast<uint16_t>(net::MsgType::OrderResponse);

    // Log received order request
    std::cout << "[SERVER] Received order request: " << body << std::endl;
    LOG_INFO("Received order request: " + body);

    // Parse JSON body
    bool success = false;
    std::string message;

    std::string symbol = extractJsonString(body, "symbol");
    int64_t qty = extractJsonInt(body, "qty");
    double price = extractJsonDouble(body, "price");
    std::string side = extractJsonString(body, "side");
    std::string type = extractJsonString(body, "type");

    // Validate required fields
    if (symbol.empty() || qty <= 0 || price <= 0) {
        message = "Invalid order parameters: symbol=" + symbol + ", qty=" + std::to_string(qty) + ", price=" + std::to_string(price);
        std::cout << "[SERVER] " << message << std::endl;
        LOG_ERROR(message);
    } else {
        // Determine order side
        order::OrderSide order_side = order::OrderSide::Buy;
        if (side == "Sell" || side == "sell" || side == "SELL") {
            order_side = order::OrderSide::Sell;
        }

        // Determine order type
        order::OrderType order_type = order::OrderType::Limit;
        if (type == "Market" || type == "market" || type == "MARKET") {
            order_type = order::OrderType::Market;
        }

        std::string cl_ord_id = "net_order_" + std::to_string(std::time(nullptr));

        std::cout << "[SERVER] Creating order: symbol=" << symbol << ", qty=" << qty
                  << ", price=" << price << ", side=" << side << std::endl;
        LOG_INFO("Creating order: symbol=" + symbol + ", qty=" + std::to_string(qty) +
                 ", price=" + std::to_string(price) + ", side=" + side);

        // Create order with parsed parameters
        auto order = std::make_shared<order::Order>(
            cl_ord_id,
            12345,  // Account ID
            symbol,
            order_side,
            order_type,
            qty
        );
        order->setPrice(price);

        // Check risk
        auto risk_results = risk_manager_->checkOrder(*order);
        bool passed = true;
        for (const auto& result : risk_results) {
            if (!result.passed) {
                passed = false;
                message = result.message;
                LOG_WARNING("Risk check failed: " + result.message);
            }
        }

        if (passed) {
            auto status = order_service_->submitOrder(order);
            if (status.ok()) {
                success = true;
                message = "Order submitted successfully, ID: " + std::to_string(order->orderId());
                std::cout << "[SERVER] " << message << std::endl;
                LOG_INFO(message);
            } else {
                message = status.toString();
                std::cout << "[SERVER] Failed to submit order: " << message << std::endl;
                LOG_ERROR("Failed to submit order: " + message);
            }
        } else {
            std::cout << "[SERVER] Risk check failed: " << message << std::endl;
        }
    }

    if (success) {
        response.body = "{\"success\":true,\"message\":\"" + message + "\",\"order_id\":" + std::to_string(0) + "}";
    } else {
        response.body = "{\"success\":false,\"message\":\"" + message + "\"}";
    }
    response.header.length = response.body.size();

    std::cout << "[SERVER] Sending response: " << response.body << std::endl;
    LOG_INFO("Sending response: " + response.body);

    return response;
}

net::Message TradeXApp::processNetworkCancel(const std::string& body) {
    net::Message response;
    response.header.magic = net::MsgHeader::MAGIC;
    response.header.type = static_cast<uint16_t>(net::MsgType::CancelResponse);

    // Parse order_id from body
    bool success = false;
    std::string message;

    if (body.find("\"order_id\"") != std::string::npos) {
        auto status = order_service_->cancelOrder(0); // Would parse actual order_id
        if (status.ok()) {
            success = true;
            message = "Order cancelled successfully";
        } else {
            message = status.toString();
        }
    } else {
        message = "Invalid cancel request format";
    }

    if (success) {
        response.body = "{\"success\":true,\"message\":\"" + message + "\"}";
    } else {
        response.body = "{\"success\":false,\"message\":\"" + message + "\"}";
    }
    response.header.length = response.body.size();

    return response;
}

net::Message TradeXApp::processNetworkQuery(const std::string& body) {
    net::Message response;
    response.header.magic = net::MsgHeader::MAGIC;
    response.header.type = static_cast<uint16_t>(net::MsgType::QueryResponse);

    // Query orders from order service
    std::ostringstream oss;
    oss << "{\"success\":true,\"orders\":[";

    // Get orders by different statuses
    auto pending = order_service_->getOrdersByStatus(order::OrderStatus::Pending);
    auto new_orders = order_service_->getOrdersByStatus(order::OrderStatus::New);
    auto partial = order_service_->getOrdersByStatus(order::OrderStatus::PartiallyFilled);
    auto filled = order_service_->getOrdersByStatus(order::OrderStatus::Filled);
    auto cancelled = order_service_->getOrdersByStatus(order::OrderStatus::Cancelled);
    auto rejected = order_service_->getOrdersByStatus(order::OrderStatus::Rejected);

    // Combine all orders
    std::vector<std::shared_ptr<order::Order>> all_orders;
    all_orders.insert(all_orders.end(), pending.begin(), pending.end());
    all_orders.insert(all_orders.end(), new_orders.begin(), new_orders.end());
    all_orders.insert(all_orders.end(), partial.begin(), partial.end());
    all_orders.insert(all_orders.end(), filled.begin(), filled.end());
    all_orders.insert(all_orders.end(), cancelled.begin(), cancelled.end());
    all_orders.insert(all_orders.end(), rejected.begin(), rejected.end());

    // Build JSON response
    bool first = true;
    for (const auto& o : all_orders) {
        if (!first) oss << ",";
        first = false;

        std::string side = (o->side() == order::OrderSide::Buy) ? "BUY" : "SELL";
        std::string status;
        switch (o->status()) {
            case order::OrderStatus::Pending: status = "Pending"; break;
            case order::OrderStatus::New: status = "New"; break;
            case order::OrderStatus::PartiallyFilled: status = "PartiallyFilled"; break;
            case order::OrderStatus::Filled: status = "Filled"; break;
            case order::OrderStatus::Cancelled: status = "Cancelled"; break;
            case order::OrderStatus::Rejected: status = "Rejected"; break;
            default: status = "Unknown"; break;
        }

        oss << "{\"order_id\":" << o->orderId()
            << ",\"symbol\":\"" << o->symbol() << "\""
            << ",\"side\":\"" << side << "\""
            << ",\"qty\":" << o->quantity()
            << ",\"filled\":" << o->quantityFilled()
            << ",\"price\":" << o->price()
            << ",\"avg_price\":" << o->avgPrice()
            << ",\"status\":\"" << status << "\"}";
    }

    oss << "],\"positions\":[]}";
    response.body = oss.str();
    response.header.length = response.body.size();

    std::cout << "[SERVER] Query orders response: " << all_orders.size() << " orders found" << std::endl;
    LOG_INFO("Query orders response: " + std::to_string(all_orders.size()) + " orders");

    return response;
}

int main(int argc, char* argv[]) {
    TradeXApp app;

    if (!app.initialize()) {
        std::cerr << "Failed to initialize application" << std::endl;
        return 1;
    }

    // Check for admin mode
    bool admin_mode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--admin" || std::string(argv[i]) == "-a") {
            admin_mode = true;
            break;
        }
    }

    if (admin_mode) {
        // Run admin CLI
        app.runAdmin();
    } else {
        // Submit a test order for demonstration
        // app.submitTestOrder(); //by liusw don't submit testorder!

        // Run the application
        app.run();
    }

    return 0;
}
