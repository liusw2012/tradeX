#include "tradex/net/net.h"
#include "tradex/framework/log.h"
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define close closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#endif

namespace tradex {
namespace net {

// Message serialization
std::vector<char> Message::serialize() const {
    std::vector<char> data(sizeof(MsgHeader) + header.length);
    std::memcpy(data.data(), &header, sizeof(MsgHeader));
    if (!body.empty()) {
        std::memcpy(data.data() + sizeof(MsgHeader), body.data(), header.length);
    }
    return data;
}

Message Message::deserialize(const char* data, size_t len) {
    Message msg;
    if (len < sizeof(MsgHeader)) {
        return msg;
    }
    std::memcpy(&msg.header, data, sizeof(MsgHeader));
    if (!msg.header.isValid()) {
        msg.header.length = 0;
        return msg;
    }
    if (msg.header.length > 0 && len >= sizeof(MsgHeader) + msg.header.length) {
        msg.body.assign(data + sizeof(MsgHeader), msg.header.length);
    }
    return msg;
}

// NetServer implementation
NetServer::NetServer()
    : port_(0), server_sock_(0), running_(false), next_client_id_(1) {
#ifdef _WIN32
    server_sock_ = (int)INVALID_SOCKET;
#endif
}

NetServer::~NetServer() {
    stop();
}

bool NetServer::start(uint16_t port) {
    port_ = port;

#ifdef _WIN32
    // Initialize Winsock
    static WSADATA wsa_data;
    static bool initialized = false;
    if (!initialized) {
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            LOG_ERROR("WSAStartup failed");
            return false;
        }
        initialized = true;
    }
#endif

    // Create socket
    server_sock_ = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (server_sock_ == INVALID_SOCKET) {
#else
    if (server_sock_ < 0) {
#endif
        LOG_ERROR("Failed to create server socket");
        return false;
    }

    // Set SO_REUSEADDR
    int opt = 1;
    setsockopt(server_sock_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    // Bind
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_sock_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("Failed to bind to port " + std::to_string(port));
        close(server_sock_);
        return false;
    }

    // Listen
    if (listen(server_sock_, 5) < 0) {
        LOG_ERROR("Failed to listen on port " + std::to_string(port));
        close(server_sock_);
        return false;
    }

    running_ = true;

    // Start accept thread
    accept_thread_ = std::thread(&NetServer::acceptLoop, this);

    LOG_INFO("Server started on port " + std::to_string(port));
    return true;
}

void NetServer::stop() {
    if (!running_) return;
    running_ = false;

#ifdef _WIN32
    if (server_sock_ != INVALID_SOCKET) {
#else
    if (server_sock_ >= 0) {
#endif
        close(server_sock_);
#ifdef _WIN32
        server_sock_ = (int)INVALID_SOCKET;
#else
        server_sock_ = -1;
#endif
    }

    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }

    // Close all client connections
    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (auto& kv : clients_) {
        close(kv.second);
    }
    clients_.clear();

    LOG_INFO("Server stopped");
}

void NetServer::acceptLoop() {
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_sock = accept(server_sock_, (struct sockaddr*)&client_addr, &addr_len);

        if (client_sock < 0) {
            if (running_) {
#ifdef _WIN32
                LOG_ERROR("Accept failed: " + std::to_string(WSAGetLastError()));
#else
                LOG_ERROR("Accept failed: " + std::string(strerror(errno)));
#endif
            }
            continue;
        }

        // Set non-blocking
#ifdef _WIN32
        u_long mode = 1;
        ioctlsocket(client_sock, FIONBIO, &mode);
#else
        int flags = fcntl(client_sock, F_GETFL, 0);
        fcntl(client_sock, F_SETFL, flags | O_NONBLOCK);
#endif

        int64_t client_id = next_client_id_++;

        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            clients_[client_id] = client_sock;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        std::string client_info = std::string(client_ip) + ":" + std::to_string(ntohs(client_addr.sin_port));
        LOG_INFO("Client connected: ID=" + std::to_string(client_id) + " Address=" + client_info);

        if (connect_cb_) {
            connect_cb_(client_id);
        }

        // Start receive thread for this client
        std::thread(&NetServer::handleClient, this, client_id, client_sock).detach();
    }
}

void NetServer::handleClient(int64_t client_id, int sock) {
    while (running_) {
        char buffer[4096];
        ssize_t n = recv(sock, buffer, sizeof(buffer), 0);

        if (n <= 0) {
#ifdef _WIN32
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                break;
            }
#else
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                break;
            }
#endif
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Try to parse message
        Message msg = Message::deserialize(buffer, n);
        if (msg.header.length > 0 && message_cb_) {
            // Log received message
            std::string msg_type;
            switch (static_cast<MsgType>(msg.header.type)) {
                case MsgType::SubmitOrder: msg_type = "SubmitOrder"; break;
                case MsgType::CancelOrder: msg_type = "CancelOrder"; break;
                case MsgType::ModifyOrder: msg_type = "ModifyOrder"; break;
                case MsgType::QueryOrders: msg_type = "QueryOrders"; break;
                case MsgType::QueryPositions: msg_type = "QueryPositions"; break;
                case MsgType::Ping: msg_type = "Ping"; break;
                default: msg_type = "Unknown"; break;
            }
            LOG_INFO("Received from client " + std::to_string(client_id) +
                     ": type=" + msg_type + " len=" + std::to_string(msg.header.length));

            message_cb_(client_id, msg);
        }
    }

    // Client disconnected
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        auto it = clients_.find(client_id);
        if (it != clients_.end()) {
            close(it->second);
            clients_.erase(it);
        }
    }

    LOG_INFO("Client disconnected: ID=" + std::to_string(client_id));

    if (disconnect_cb_) {
        disconnect_cb_(client_id);
    }
}

bool NetServer::sendTo(int64_t client_id, const Message& msg) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    auto it = clients_.find(client_id);
    if (it == clients_.end()) {
        LOG_WARNING("sendTo failed: client " + std::to_string(client_id) + " not found");
        return false;
    }

    auto data = msg.serialize();
    ssize_t n = send(it->second, (const char*)data.data(), data.size(), 0);

    // Log sent message
    std::string msg_type;
    switch (static_cast<MsgType>(msg.header.type)) {
        case MsgType::OrderResponse: msg_type = "OrderResponse"; break;
        case MsgType::CancelResponse: msg_type = "CancelResponse"; break;
        case MsgType::QueryResponse: msg_type = "QueryResponse"; break;
        case MsgType::PositionsResponse: msg_type = "PositionsResponse"; break;
        case MsgType::Pong: msg_type = "Pong"; break;
        case MsgType::ErrorResponse: msg_type = "ErrorResponse"; break;
        default: msg_type = "Unknown"; break;
    }
    LOG_INFO("Sent to client " + std::to_string(client_id) +
             ": type=" + msg_type + " len=" + std::to_string(msg.header.length));

    return n == (ssize_t)data.size();
}

bool NetServer::broadcast(const Message& msg) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    auto data = msg.serialize();

    bool all_ok = true;
    for (auto& kv : clients_) {
        ssize_t n = send(kv.second, (const char*)data.data(), data.size(), 0);
        if (n != (ssize_t)data.size()) {
            all_ok = false;
        }
    }
    return all_ok;
}

// NetClient implementation
NetClient::NetClient()
    : port_(0), sock_(0), running_(false), connected_(false) {
#ifdef _WIN32
    sock_ = (int)INVALID_SOCKET;
#else
    sock_ = -1;
#endif
}

NetClient::~NetClient() {
    disconnect();
}

bool NetClient::connect(const std::string& host, uint16_t port) {
    host_ = host;
    port_ = port;

#ifdef _WIN32
    // Initialize Winsock
    static WSADATA wsa_data;
    static bool initialized = false;
    if (!initialized) {
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            LOG_ERROR("WSAStartup failed");
            return false;
        }
        initialized = true;
    }
#endif

    // Create socket
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (sock_ == INVALID_SOCKET) {
#else
    if (sock_ < 0) {
#endif
        LOG_ERROR("Failed to create socket");
        return false;
    }

    // Resolve host
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        // Try to resolve hostname
        struct hostent* he = gethostbyname(host.c_str());
        if (!he) {
            LOG_ERROR("Failed to resolve host: " + host);
            close(sock_);
            return false;
        }
        std::memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    // Connect
    if (::connect(sock_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("Failed to connect to " + host + ":" + std::to_string(port));
        close(sock_);
        return false;
    }

    running_ = true;
    connected_ = true;

    // Start receive thread
    receive_thread_ = std::thread(&NetClient::receiveLoop, this);

    LOG_INFO("Connected to " + host + ":" + std::to_string(port));

    if (connected_cb_) {
        connected_cb_();
    }

    return true;
}

void NetClient::disconnect() {
    if (!running_) return;
    running_ = false;
    connected_ = false;

#ifdef _WIN32
    if (sock_ != INVALID_SOCKET) {
#else
    if (sock_ >= 0) {
#endif
        close(sock_);
#ifdef _WIN32
        sock_ = (int)INVALID_SOCKET;
#else
        sock_ = -1;
#endif
    }

    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }

    LOG_INFO("Disconnected from server");

    if (disconnected_cb_) {
        disconnected_cb_();
    }
}

void NetClient::receiveLoop() {
    while (running_ && connected_) {
        char buffer[4096];
        ssize_t n = recv(sock_, buffer, sizeof(buffer), 0);

        if (n <= 0) {
            if (running_) {
                connected_ = false;
                LOG_ERROR("Connection lost");
                if (disconnected_cb_) {
                    disconnected_cb_();
                }
            }
            break;
        }

        Message msg = Message::deserialize(buffer, n);
        if (msg.header.length > 0 && message_cb_) {
            message_cb_(msg);
        }
    }
}

bool NetClient::send(const Message& msg) {
    if (!connected_) return false;

    auto data = msg.serialize();
    std::lock_guard<std::mutex> lock(send_mutex_);
#ifdef _WIN32
    ssize_t n = ::send(sock_, (const char*)data.data(), (int)data.size(), 0);
#else
    ssize_t n = ::send(sock_, (const char*)data.data(), data.size(), 0);
#endif
    return n == (ssize_t)data.size();
}

bool NetClient::receive(Message& msg, int timeout_ms) {
    // This is a simplified blocking receive
    // In production, use a proper message queue
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
    return false;  // Messages are delivered via callback
}

} // namespace net
} // namespace tradex