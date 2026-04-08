#include "tradex/order/order.h"
#include <sstream>
#include <iomanip>

namespace tradex {
namespace order {

Order::Order()
    : order_id_(0), account_id_(0),
      side_(OrderSide::Buy), type_(OrderType::Limit), tif_(TimeInForce::Day),
      quantity_(0), quantity_filled_(0),
      price_(0.0), stop_price_(0.0), avg_price_(0.0),
      status_(OrderStatus::Pending), source_(OrderSource::Manual),
      create_time_(0), update_time_(0), expire_time_(0), gateway_id_(0) {}

Order::Order(const std::string& cl_ord_id, int64_t account_id,
             const std::string& symbol, OrderSide side,
             OrderType type, int64_t quantity)
    : order_id_(0), cl_ord_id_(cl_ord_id), account_id_(account_id),
      symbol_(symbol), side_(side), type_(type), tif_(TimeInForce::Day),
      quantity_(quantity), quantity_filled_(0),
      price_(0.0), stop_price_(0.0), avg_price_(0.0),
      status_(OrderStatus::Pending), source_(OrderSource::Manual),
      create_time_(0), update_time_(0), expire_time_(0), gateway_id_(0) {}

void Order::setStatus(OrderStatus status) {
    status_ = status;
    update_time_ = static_cast<int64_t>(time(nullptr));
}

void Order::setCreateTime(int64_t time) {
    create_time_ = time;
}

void Order::setUpdateTime(int64_t time) {
    update_time_ = time;
}

std::string Order::toString() const {
    std::ostringstream oss;
    oss << "Order{"
        << "id=" << order_id_
        << ", cl_ord_id=" << cl_ord_id_
        << ", account=" << account_id_
        << ", symbol=" << symbol_
        << ", side=" << (side_ == OrderSide::Buy ? "BUY" : "SELL")
        << ", type=" << static_cast<int>(type_)
        << ", quantity=" << quantity_
        << ", filled=" << quantity_filled_
        << ", price=" << std::fixed << std::setprecision(2) << price_
        << ", status=" << static_cast<int>(status_)
        << "}";
    return oss.str();
}

// Helper functions
std::string orderSideToString(OrderSide side) {
    return side == OrderSide::Buy ? "BUY" : "SELL";
}

std::string orderTypeToString(OrderType type) {
    switch (type) {
        case OrderType::Market: return "MARKET";
        case OrderType::Limit: return "LIMIT";
        case OrderType::Stop: return "STOP";
        case OrderType::StopLimit: return "STOP_LIMIT";
        default: return "UNKNOWN";
    }
}

std::string orderStatusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::Pending: return "PENDING";
        case OrderStatus::New: return "NEW";
        case OrderStatus::PartiallyFilled: return "PARTIALLY_FILLED";
        case OrderStatus::Filled: return "FILLED";
        case OrderStatus::CancelPending: return "CANCEL_PENDING";
        case OrderStatus::Cancelled: return "CANCELLED";
        case OrderStatus::ReplacePending: return "REPLACE_PENDING";
        case OrderStatus::Replaced: return "REPLACED";
        case OrderStatus::Rejected: return "REJECTED";
        case OrderStatus::Expired: return "EXPIRED";
        default: return "UNKNOWN";
    }
}

} // namespace order
} // namespace tradex