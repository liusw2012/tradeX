#pragma once

#include "tradex/order/order.h"
#include "tradex/framework/status.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <unordered_map>

namespace tradex {
namespace order {

// Order service interface
class IOrderService {
public:
    virtual ~IOrderService() = default;

    // Order operations
    virtual framework::Status submitOrder(std::shared_ptr<Order> order) = 0;
    virtual framework::Status cancelOrder(int64_t order_id) = 0;
    virtual framework::Status modifyOrder(int64_t order_id,
                                           int64_t new_quantity,
                                           double new_price) = 0;

    // Query
    virtual std::shared_ptr<Order> getOrder(int64_t order_id) = 0;
    virtual std::shared_ptr<Order> getOrderByClOrdId(const std::string& cl_ord_id) = 0;
    virtual std::vector<std::shared_ptr<Order>> getOrdersByAccount(int64_t account_id) = 0;
    virtual std::vector<std::shared_ptr<Order>> getOrdersByStatus(OrderStatus status) = 0;
};

// Order manager - core order management
class OrderService : public IOrderService {
public:
    OrderService();
    ~OrderService() override;

    // Initialize
    bool initialize();

    // Register update listener
    void setUpdateListener(OrderUpdateListener listener);

    // Submit order
    framework::Status submitOrder(std::shared_ptr<Order> order) override;

    // Cancel order
    framework::Status cancelOrder(int64_t order_id) override;

    // Modify order
    framework::Status modifyOrder(int64_t order_id,
                                   int64_t new_quantity,
                                   double new_price) override;

    // Query
    std::shared_ptr<Order> getOrder(int64_t order_id) override;
    std::shared_ptr<Order> getOrderByClOrdId(const std::string& cl_ord_id) override;
    std::vector<std::shared_ptr<Order>> getOrdersByAccount(int64_t account_id) override;
    std::vector<std::shared_ptr<Order>> getOrdersByStatus(OrderStatus status) override;

    // Internal methods for trade execution
    void onOrderFill(int64_t order_id, int64_t fill_quantity, double fill_price);
    void onOrderReject(int64_t order_id, const std::string& reason);

    // Statistics
    size_t totalOrders() const;
    size_t pendingOrders() const;
    size_t activeOrders() const;

private:
    framework::Status doSubmitOrder(std::shared_ptr<Order> order);
    bool doCancelOrder(int64_t order_id);
    bool doModifyOrder(int64_t order_id, int64_t new_quantity, double new_price);

    void notifyUpdate(const OrderUpdate& update);

    int64_t generateOrderId();

    mutable std::mutex mutex_;
    int64_t next_order_id_;

    std::unordered_map<int64_t, std::shared_ptr<Order>> orders_by_id_;
    std::unordered_map<std::string, std::shared_ptr<Order>> orders_by_cl_ord_id_;

    OrderUpdateListener listener_;
};

// Order repository interface for persistence
class IOrderRepository {
public:
    virtual ~IOrderRepository() = default;

    virtual bool save(const Order& order) = 0;
    virtual bool update(const Order& order) = 0;
    virtual bool remove(int64_t order_id) = 0;
    virtual std::unique_ptr<Order> findById(int64_t order_id) = 0;
    virtual std::vector<Order> findByAccount(int64_t account_id) = 0;
    virtual std::vector<Order> findByStatus(OrderStatus status) = 0;
};

// In-memory order repository
class InMemoryOrderRepository : public IOrderRepository {
public:
    InMemoryOrderRepository() = default;

    bool save(const Order& order) override;
    bool update(const Order& order) override;
    bool remove(int64_t order_id) override;
    std::unique_ptr<Order> findById(int64_t order_id) override;
    std::vector<Order> findByAccount(int64_t account_id) override;
    std::vector<Order> findByStatus(OrderStatus status) override;

private:
    std::unordered_map<int64_t, Order> orders_;
    mutable std::mutex mutex_;
};

} // namespace order
} // namespace tradex