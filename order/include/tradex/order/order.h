#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <functional>

namespace tradex {
namespace order {

// Order side
enum class OrderSide {
    Buy = 1,
    Sell = 2
};

// Order type
enum class OrderType {
    Market = 1,    // Market order
    Limit = 2,     // Limit order
    Stop = 3,     // Stop order
    StopLimit = 4 // Stop limit order
};

// Order time condition
enum class TimeInForce {
    Day = 1,       // Day order
    GTC = 2,       // Good till cancelled
    IOC = 3,       // Immediate or cancel
    FOK = 4,       // Fill or kill
    GTD = 5        // Good till date
};

// Order status
enum class OrderStatus {
    Pending = 1,       // Submitted, waiting for processing
    New = 2,           // Accepted by exchange
    PartiallyFilled = 3,
    Filled = 4,        // Fully filled
    CancelPending = 5, // Cancel submitted
    Cancelled = 6,
    ReplacePending = 7, // Modify submitted
    Replaced = 8,
    Rejected = 9,
    Expired = 10
};

// Order source
enum class OrderSource {
    Manual = 1,    // Manual trading
    Algo = 2,      // Algorithm trading
    Strategy = 3,  // Strategy trading
    AI = 4         // AI trading
};

// Order entity
class Order {
public:
    Order();
    Order(const std::string& cl_ord_id, int64_t account_id,
          const std::string& symbol, OrderSide side,
          OrderType type, int64_t quantity);

    // Getters
    int64_t orderId() const { return order_id_; }
    const std::string& clOrdId() const { return cl_ord_id_; }
    int64_t accountId() const { return account_id_; }
    const std::string& symbol() const { return symbol_; }
    OrderSide side() const { return side_; }
    OrderType type() const { return type_; }
    TimeInForce timeInForce() const { return tif_; }

    int64_t quantity() const { return quantity_; }
    int64_t quantityFilled() const { return quantity_filled_; }
    int64_t quantityRemaining() const { return quantity_ - quantity_filled_; }

    double price() const { return price_; }
    double stopPrice() const { return stop_price_; }
    double avgPrice() const { return avg_price_; }

    OrderStatus status() const { return status_; }
    OrderSource source() const { return source_; }

    int64_t createTime() const { return create_time_; }
    int64_t updateTime() const { return update_time_; }
    int64_t expireTime() const { return expire_time_; }

    const std::string& rejectReason() const { return reject_reason_; }
    int64_t gatewayId() const { return gateway_id_; }

    // Setters
    void setOrderId(int64_t id) { order_id_ = id; }
    void setClOrdId(const std::string& id) { cl_ord_id_ = id; }
    void setAccountId(int64_t id) { account_id_ = id; }
    void setSymbol(const std::string& s) { symbol_ = s; }
    void setSide(OrderSide s) { side_ = s; }
    void setType(OrderType t) { type_ = t; }
    void setTimeInForce(TimeInForce tif) { tif_ = tif; }

    void setQuantity(int64_t qty) { quantity_ = qty; }
    void setQuantityFilled(int64_t qty) { quantity_filled_ = qty; }
    void setPrice(double price) { price_ = price; }
    void setStopPrice(double price) { stop_price_ = price; }
    void setAvgPrice(double price) { avg_price_ = price; }

    void setStatus(OrderStatus status);
    void setSource(OrderSource src) { source_ = src; }

    void setCreateTime(int64_t time);
    void setUpdateTime(int64_t time);
    void setExpireTime(int64_t time) { expire_time_ = time; }

    void setRejectReason(const std::string& reason) { reject_reason_ = reason; }
    void setGatewayId(int64_t id) { gateway_id_ = id; }

    // Helpers
    bool isFilled() const { return status_ == OrderStatus::Filled; }
    bool isCancelled() const { return status_ == OrderStatus::Cancelled; }
    bool isRejected() const { return status_ == OrderStatus::Rejected; }
    bool isTerminal() const {
        return status_ == OrderStatus::Filled ||
               status_ == OrderStatus::Cancelled ||
               status_ == OrderStatus::Rejected ||
               status_ == OrderStatus::Expired;
    }

    std::string toString() const;

private:
    int64_t order_id_;
    std::string cl_ord_id_;      // Client order ID
    int64_t account_id_;
    std::string symbol_;

    OrderSide side_;
    OrderType type_;
    TimeInForce tif_;

    int64_t quantity_;           // Original quantity
    int64_t quantity_filled_;    // Filled quantity

    double price_;               // Limit price
    double stop_price_;          // Stop price
    double avg_price_;          // Average fill price

    OrderStatus status_;
    OrderSource source_;

    int64_t create_time_;
    int64_t update_time_;
    int64_t expire_time_;

    std::string reject_reason_;
    int64_t gateway_id_;
};

// Order comparator for sorting
struct OrderComparator {
    bool operator()(const std::shared_ptr<Order>& a,
                    const std::shared_ptr<Order>& b) const {
        return a->orderId() < b->orderId();
    }
};

// Order update event
struct OrderUpdate {
    enum class UpdateType {
        New,
        Fill,
        Cancel,
        Reject,
        Modify
    };

    int64_t order_id;
    OrderUpdate::UpdateType update_type;
    int64_t quantity_filled;
    double avg_price;
    OrderStatus new_status;
    std::string message;
};

// Order update listener
using OrderUpdateListener = std::function<void(const OrderUpdate&)>;

} // namespace order
} // namespace tradex