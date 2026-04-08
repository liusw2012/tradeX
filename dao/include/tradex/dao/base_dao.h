#pragma once

#include "tradex/dao/connection_pool.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace tradex {
namespace dao {

// Result set wrapper
class ResultSet {
public:
    ResultSet() : rows_(0), cols_(0), data_(nullptr) {}
    ~ResultSet();

    bool hasData() const { return data_ != nullptr && rows_ > 0; }
    int rows() const { return rows_; }
    int cols() const { return cols_; }

    std::string getString(int row, int col) const;
    int getInt(int row, int col) const;
    int64_t getInt64(int row, int col) const;
    double getDouble(int row, int col) const;

    std::string getString(int row, const std::string& col_name) const;
    int getInt(int row, const std::string& col_name) const;
    int64_t getInt64(int row, const std::string& col_name) const;
    double getDouble(int row, const std::string& col_name) const;

    // For internal use
    void setData(void* data, int rows, int cols,
                 const std::vector<std::string>& col_names);

private:
    int rows_;
    int cols_;
    void* data_;
    std::vector<std::string> col_names_;
    std::map<std::string, int> col_name_map_;
};

// Base DAO template
template<typename T>
class BaseDAO {
public:
    BaseDAO() : pool_(nullptr) {}
    explicit BaseDAO(ConnectionPool* pool) : pool_(pool) {}

    void setConnectionPool(ConnectionPool* pool) { pool_ = pool; }

    // CRUD operations
    virtual bool insert(const T& entity) = 0;
    virtual bool update(const T& entity) = 0;
    virtual bool remove(int64_t id) = 0;
    virtual std::unique_ptr<T> findById(int64_t id) = 0;
    virtual std::vector<T> findAll() = 0;

    // Query with conditions
    virtual std::vector<T> findByCondition(const std::string& where,
                                           const std::vector<std::string>& params) = 0;

protected:
    ConnectionPool* pool_;

    std::shared_ptr<Connection> getConnection() {
        if (pool_) {
            return pool_->getConnection();
        }
        return nullptr;
    }

    std::string escapeString(const std::string& str) const {
        auto conn = getConnection();
        if (conn) {
            return conn->escapeString(str);
        }
        return str;
    }
};

// Transaction manager
class Transaction {
public:
    explicit Transaction(ConnectionPool* pool);
    ~Transaction();

    bool begin();
    bool commit();
    bool rollback();

    bool isActive() const { return active_ && conn_ && conn_->inTransaction(); }
    Connection* connection() { return conn_.get(); }

private:
    ConnectionPool* pool_;
    std::shared_ptr<Connection> conn_;
    bool active_;
};

// DAO exception
class DAOException : public std::runtime_error {
public:
    explicit DAOException(const std::string& msg)
        : std::runtime_error(msg) {}
};

// Helper macro for SQL building
#define BUILD_INSERT(table, fields) \
    "INSERT INTO " table " (" #fields ") VALUES (")

#define BUILD_UPDATE(table, id_field) \
    "UPDATE " table " SET "

#define BUILD_DELETE(table, id_field) \
    "DELETE FROM " table " WHERE " #id_field " = "

#define BUILD_SELECT(table) \
    "SELECT * FROM " table

} // namespace dao
} // namespace tradex