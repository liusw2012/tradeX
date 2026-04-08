// tradeX CLI Client - Network-based trading interface
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <csignal>
#include <thread>
#include <chrono>
#include <atomic>

#include "tradex/net/net.h"
#include "tradex/order/order.h"
#include "tradex/framework/log.h"
#include "tradex/framework/config.h"

using namespace tradex;

static bool g_running = true;
static std::unique_ptr<net::NetClient> g_client;
static std::atomic<int64_t> g_request_id(1);

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nDisconnecting..." << std::endl;
        g_running = false;
    }
}

void printBanner() {
    std::cout << "\n";
    std::cout << "========================================" << std::endl;
    std::cout << "     tradeX Trading CLI Client         " << std::endl;
    std::cout << "========================================" << std::endl;
}

void printHelp() {
    std::cout << "\nAvailable Commands:" << std::endl;
    std::cout << "-------------------" << std::endl;
    std::cout << "  Connection:" << std::endl;
    std::cout << "    connect <host> <port>  - Connect to server" << std::endl;
    std::cout << "    disconnect            - Disconnect from server" << std::endl;
    std::cout << "    status                - Show connection status" << std::endl;
    std::cout << std::endl;
    std::cout << "  Order Commands:" << std::endl;
    std::cout << "    buy <symbol> <qty> <price>   - Submit buy order" << std::endl;
    std::cout << "    sell <symbol> <qty> <price>  - Submit sell order" << std::endl;
    std::cout << "    cancel <order_id>             - Cancel an order" << std::endl;
    std::cout << "    order <order_id>              - Query order details" << std::endl;
    std::cout << "    orders                         - List all orders" << std::endl;
    std::cout << std::endl;
    std::cout << "  Position Commands:" << std::endl;
    std::cout << "    positions                      - List all positions" << std::endl;
    std::cout << "    trades                         - List recent trades" << std::endl;
    std::cout << "    account                        - Show account info" << std::endl;
    std::cout << std::endl;
    std::cout << "  Market Commands:" << std::endl;
    std::cout << "    quote <symbol>                - Get quote" << std::endl;
    std::cout << "    subscribe <symbol>             - Subscribe to quote" << std::endl;
    std::cout << std::endl;
    std::cout << "  System Commands:" << std::endl;
    std::cout << "    help                           - Show this help" << std::endl;
    std::cout << "    quit/exit                     - Exit CLI" << std::endl;
    std::cout << "-------------------" << std::endl;
}

void printConnectionStatus() {
    if (g_client && g_client->isConnected()) {
        std::cout << "Connection: CONNECTED" << std::endl;
    } else {
        std::cout << "Connection: DISCONNECTED" << std::endl;
        std::cout << "Use 'connect <host> <port>' to connect to tradeX server" << std::endl;
    }
}

std::string processCommand(const std::string& line) {
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    if (cmd.empty()) {
        return "";
    }

    // Connection commands - don't need server connection
    if (cmd == "help" || cmd == "?") {
        printHelp();
        return "";
    } else if (cmd == "clear" || cmd == "cls") {
        return "\033[2J\033[1;1H";
    } else if (cmd == "quit" || cmd == "exit" || cmd == "q") {
        if (g_client) {
            g_client->disconnect();
        }
        g_running = false;
        return "Goodbye!";
    } else if (cmd == "status") {
        printConnectionStatus();
        return "";
    } else if (cmd == "connect") {
        std::string host;
        uint16_t port;
        if (!(iss >> host >> port)) {
            return "Usage: connect <host> <port>\nExample: connect 127.0.0.1 9000";
        }

        g_client = std::make_unique<net::NetClient>();
        if (g_client->connect(host, port)) {
            return "Connected to " + host + ":" + std::to_string(port);
        } else {
            g_client.reset();
            return "Failed to connect to " + host + ":" + std::to_string(port);
        }
    } else if (cmd == "disconnect") {
        if (g_client) {
            g_client->disconnect();
            g_client.reset();
            return "Disconnected";
        }
        return "Not connected";
    }

    // Check connection for other commands
    if (!g_client || !g_client->isConnected()) {
        return "Error: Not connected to server\nUse 'connect <host> <port>' to connect";
    }

    try {
        if (cmd == "buy") {
            std::string symbol;
            int64_t qty;
            double price;
            if (iss >> symbol >> qty >> price) {
                int64_t req_id = g_request_id++;
                auto msg = net::Protocol::createSubmitOrder(req_id, symbol, qty, price, "Buy", "Limit");
                g_client->send(msg);
                return "Order submitted: BUY " + symbol + " " + std::to_string(qty) +
                       " @ " + std::to_string(price) + " [RequestID: " + std::to_string(req_id) + "]";
            }
            return "Usage: buy <symbol> <qty> <price>\nExample: buy 600000 100 10.50";
        } else if (cmd == "sell") {
            std::string symbol;
            int64_t qty;
            double price;
            if (iss >> symbol >> qty >> price) {
                int64_t req_id = g_request_id++;
                auto msg = net::Protocol::createSubmitOrder(req_id, symbol, qty, price, "Sell", "Limit");
                g_client->send(msg);
                return "Order submitted: SELL " + symbol + " " + std::to_string(qty) +
                       " @ " + std::to_string(price) + " [RequestID: " + std::to_string(req_id) + "]";
            }
            return "Usage: sell <symbol> <qty> <price>";
        } else if (cmd == "cancel") {
            std::string order_id;
            if (iss >> order_id) {
                int64_t req_id = g_request_id++;
                auto msg = net::Protocol::createCancelOrder(req_id, std::stoll(order_id));
                g_client->send(msg);
                return "Cancel request sent for order " + order_id;
            }
            return "Usage: cancel <order_id>";
        } else if (cmd == "orders") {
            int64_t req_id = g_request_id++;
            auto msg = net::Protocol::createQueryOrders(req_id);
            g_client->send(msg);
            return "Query sent for orders [RequestID: " + std::to_string(req_id) + "]";
        } else if (cmd == "positions") {
            int64_t req_id = g_request_id++;
            auto msg = net::Protocol::createQueryPositions(req_id, 100);
            g_client->send(msg);
            return "Query sent for positions [RequestID: " + std::to_string(req_id) + "]";
        } else if (cmd == "account") {
            return "Account: ID=100, Cash=1000000.00, Available=1000000.00";
        } else if (cmd == "quote") {
            std::string symbol;
            if (iss >> symbol) {
                return "Quote for " + symbol + ":\n  Last: 10.50\n  Bid: 10.48\n  Ask: 10.52";
            }
            return "Usage: quote <symbol>";
        } else if (cmd == "subscribe") {
            std::string symbol;
            if (iss >> symbol) {
                return "Subscribed to " + symbol;
            }
            return "Usage: subscribe <symbol>";
        } else if (cmd == "ping") {
            int64_t req_id = g_request_id++;
            auto msg = net::Protocol::createPing(req_id);
            g_client->send(msg);
            return "Ping sent [RequestID: " + std::to_string(req_id) + "]";
        } else {
            return "Unknown command: " + cmd + "\nType 'help' for available commands";
        }
    } catch (const std::exception& e) {
        return "Error: " + std::string(e.what());
    }
}

void runInteractive() {
    printBanner();
    printConnectionStatus();

    std::string line;
    while (g_running) {
        std::cout << "\n[tradeX] $ ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line.empty()) {
            continue;
        }

        std::string result = processCommand(line);
        if (!result.empty()) {
            std::cout << result << std::endl;
        }
    }
}

void runCommand(const std::string& cmd) {
    std::string result = processCommand(cmd);
    if (!result.empty()) {
        std::cout << result << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // Setup signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Check for connect command in args
    if (argc >= 3 && std::string(argv[1]) == "connect") {
        std::string host = argv[2];
        uint16_t port = std::stoi(argv[3]);

        std::cout << "Connecting to " << host << ":" << port << "..." << std::endl;
        g_client = std::make_unique<net::NetClient>();
        if (g_client->connect(host, port)) {
            std::cout << "Connected!" << std::endl;

            // If additional command provided, send it
            if (argc > 4) {
                std::string cmd;
                for (int i = 4; i < argc; ++i) {
                    cmd += argv[i];
                    cmd += " ";
                }
                runCommand(cmd);
            }

            // Run interactive if connected without additional commands
            if (argc <= 4) {
                runInteractive();
            }
        } else {
            std::cout << "Failed to connect!" << std::endl;
            return 1;
        }
    } else if (argc > 1) {
        // Command mode - execute and exit
        std::string cmd;
        for (int i = 1; i < argc; ++i) {
            cmd += argv[i];
            cmd += " ";
        }
        runCommand(cmd);
    } else {
        // Interactive mode
        runInteractive();
    }

    return 0;
}