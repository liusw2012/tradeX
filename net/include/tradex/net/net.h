#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <cstdint>
#include <map>

namespace tradex {
namespace net {

// Message types
enum class MsgType : uint16_t {
    // Client -> Server
    SubmitOrder = 1,
    CancelOrder = 2,
    ModifyOrder = 3,
    QueryOrder = 4,
    QueryOrders = 5,
    QueryPosition = 6,
    QueryPositions = 7,
    QueryTrade = 8,
    QueryTrades = 9,
    QueryAccount = 10,
    Subscribe = 11,
    Unsubscribe = 12,

    // Server -> Client
    OrderResponse = 101,
    CancelResponse = 102,
    ModifyResponse = 103,
    QueryResponse = 104,
    PositionsResponse = 105,
    TradesResponse = 106,
    AccountResponse = 107,
    QuoteUpdate = 108,
    ErrorResponse = 109,

    // System
    Ping = 200,
    Pong = 201,
    Connect = 202,
    Disconnect = 203
};

// Message header
struct MsgHeader {
    uint32_t magic;        // Magic number for validation
    uint32_t length;      // Message body length
    uint16_t type;        // Message type
    uint16_t sequence;   // Sequence number
    int64_t timestamp;    // Timestamp

    static constexpr uint32_t MAGIC = 0x54584E45; // "TRXNE"

    bool isValid() const {
        return magic == MAGIC && length < 1024 * 1024; // Max 1MB
    }
};

// Message structure
struct Message {
    MsgHeader header;
    std::string body;

    std::vector<char> serialize() const;
    static Message deserialize(const char* data, size_t len);
};

// Request/Response base
struct Request {
    int64_t request_id;
    MsgType type;
    std::string data;
};

struct Response {
    int64_t request_id;
    bool success;
    std::string message;
    std::string data;
};

// Network server
class NetServer {
public:
    using ConnectCallback = std::function<void(int64_t client_id)>;
    using DisconnectCallback = std::function<void(int64_t client_id)>;
    using MessageCallback = std::function<void(int64_t client_id, const Message&)>;

    NetServer();
    ~NetServer();

    // Start server on port
    bool start(uint16_t port);
    void stop();

    // Send message to client
    bool sendTo(int64_t client_id, const Message& msg);
    bool broadcast(const Message& msg);

    // Callbacks
    void setConnectCallback(ConnectCallback cb) { connect_cb_ = cb; }
    void setDisconnectCallback(DisconnectCallback cb) { disconnect_cb_ = cb; }
    void setMessageCallback(MessageCallback cb) { message_cb_ = cb; }

    bool isRunning() const { return running_; }
    uint16_t port() const { return port_; }

private:
    void acceptLoop();
    void handleClient(int64_t client_id, int sock);
    void receiveLoop(int64_t client_id, int sock);

    uint16_t port_;
    int server_sock_;
    std::atomic<bool> running_;
    std::thread accept_thread_;

    ConnectCallback connect_cb_;
    DisconnectCallback disconnect_cb_;
    MessageCallback message_cb_;

    std::mutex clients_mutex_;
    std::map<int64_t, int> clients_;
    std::atomic<int64_t> next_client_id_;
};

// Network client
class NetClient {
public:
    using ConnectedCallback = std::function<void()>;
    using DisconnectedCallback = std::function<void()>;
    using MessageCallback = std::function<void(const Message&)>;

    NetClient();
    ~NetClient();

    // Connect to server
    bool connect(const std::string& host, uint16_t port);
    void disconnect();

    // Send message
    bool send(const Message& msg);

    // Receive message (blocking)
    bool receive(Message& msg, int timeout_ms = 5000);

    // Callbacks
    void setConnectedCallback(ConnectedCallback cb) { connected_cb_ = cb; }
    void setDisconnectedCallback(DisconnectedCallback cb) { disconnected_cb_ = cb; }
    void setMessageCallback(MessageCallback cb) { message_cb_ = cb; }

    bool isConnected() const { return connected_; }

private:
    void receiveLoop();

    std::string host_;
    uint16_t port_;
    int sock_;
    std::atomic<bool> running_;
    std::atomic<bool> connected_;
    std::thread receive_thread_;

    ConnectedCallback connected_cb_;
    DisconnectedCallback disconnected_cb_;
    MessageCallback message_cb_;

    std::mutex send_mutex_;
};

// Protocol helpers
class Protocol {
public:
    // Create request messages
    static Message createSubmitOrder(int64_t request_id,
        const std::string& symbol, int64_t qty, double price,
        const std::string& side, const std::string& type);

    static Message createCancelOrder(int64_t request_id, int64_t order_id);
    static Message createQueryOrders(int64_t request_id);
    static Message createQueryPositions(int64_t request_id, int64_t account_id);
    static Message createPing(int64_t request_id);

    // Parse response messages
    static bool parseOrderResponse(const Message& msg, std::string& order_id,
        bool& success, std::string& message);
    static bool parsePositionsResponse(const Message& msg, std::string& positions_json);
};

} // namespace net
} // namespace tradex