#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

namespace tradex {
namespace dao {

// Database connection interface
class Connection {
public:
    virtual ~Connection() = default;

    virtual bool connect(const std::string& host, int port,
                         const std::string& user,
                         const std::string& password,
                         const std::string& database) = 0;

    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    virtual bool execute(const std::string& sql) = 0;
    virtual bool query(const std::string& sql, void** result, int* rows, int* cols) = 0;
    virtual std::string escapeString(const std::string& str) const = 0;

    virtual int64_t lastInsertId() const = 0;
    virtual int affectedRows() const = 0;
    virtual std::string error() const = 0;

    virtual void beginTransaction() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;
    virtual bool inTransaction() const = 0;
};

// Connection pool
class ConnectionPool {
public:
    ConnectionPool();
    ~ConnectionPool();

    // Initialize pool
    bool initialize(const std::string& host, int port,
                    const std::string& user,
                    const std::string& password,
                    const std::string& database,
                    size_t pool_size);

    // Get connection from pool
    std::shared_ptr<Connection> getConnection();

    // Return connection to pool
    void returnConnection(std::shared_ptr<Connection> conn);

    // Shutdown pool
    void shutdown();

    // Pool statistics
    size_t poolSize() const { return pool_size_; }
    size_t availableConnections() const;
    size_t inUseConnections() const;

private:
    std::string host_;
    int port_;
    std::string user_;
    std::string password_;
    std::string database_;
    size_t pool_size_;

    std::queue<std::shared_ptr<Connection>> available_conns_;
    std::atomic<size_t> in_use_count_;

    std::mutex mutex_;
    std::condition_variable condition_;
    std::atomic<bool> shutdown_;

    std::shared_ptr<Connection> createConnection();
};

// Global connection pool
ConnectionPool& globalConnectionPool();

} // namespace dao
} // namespace tradex