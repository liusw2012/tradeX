#include "tradex/order/order_service.h"
#include "tradex/framework/log.h"
#include <chrono>

namespace tradex {
namespace order {

OrderService::OrderService() : next_order_id_(1000000) {}

OrderService::~OrderService() = default;

bool OrderService::initialize() {
    LOG_INFO("OrderService initialized");
    return true;
}

void OrderService::setUpdateListener(OrderUpdateListener listener) {
    listener_ = listener;
}

int64_t OrderService::generateOrderId() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    return timestamp + (next_order_id_++ % 10000);
}

framework::Status OrderService::submitOrder(std::shared_ptr<Order> order) {
    if (!order) {
        return framework::Status::InvalidArgument("Order is null");
    }

    if (order->symbol().empty()) {
        return framework::Status::InvalidArgument("Symbol is empty");
    }

    if (order->quantity() <= 0) {
        return framework::Status::InvalidArgument("Quantity must be positive");
    }

    if (order->type() == OrderType::Limit && order->price() <= 0) {
        return framework::Status::InvalidArgument("Limit price must be positive");
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Check for duplicate cl_ord_id
    if (!order->clOrdId().empty()) {
        if (orders_by_cl_ord_id_.find(order->clOrdId()) != orders_by_cl_ord_id_.end()) {
            return framework::Status::AlreadyExists("ClOrdId already exists: " + order->clOrdId());
        }
    }

    return doSubmitOrder(order);
}

framework::Status OrderService::doSubmitOrder(std::shared_ptr<Order> order) {
    // Generate order ID
    int64_t order_id = generateOrderId();
    order->setOrderId(order_id);
    order->setStatus(OrderStatus::New);
    order->setCreateTime(std::time(nullptr));
    order->setUpdateTime(std::time(nullptr));

    // Store in maps
    orders_by_id_[order_id] = order;
    if (!order->clOrdId().empty()) {
        orders_by_cl_ord_id_[order->clOrdId()] = order;
    }

    LOG_INFO("Order submitted: " + order->toString());

    // Notify update
    OrderUpdate update;
    update.order_id = order_id;
    update.update_type = OrderUpdate::UpdateType::New;
    update.quantity_filled = 0;
    update.avg_price = 0.0;
    update.new_status = order->status();
    update.message = "Order submitted";
    notifyUpdate(update);

    return framework::Status::OK();
}

framework::Status OrderService::cancelOrder(int64_t order_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!doCancelOrder(order_id)) {
        return framework::Status::NotFound("Order not found: " + std::to_string(order_id));
    }

    return framework::Status::OK();
}

bool OrderService::doCancelOrder(int64_t order_id) {
    auto it = orders_by_id_.find(order_id);
    if (it == orders_by_id_.end()) {
        return false;
    }

    auto order = it->second;
    if (order->isTerminal()) {
        return false;  // Cannot cancel terminal orders
    }

    order->setStatus(OrderStatus::Cancelled);
    order->setUpdateTime(std::time(nullptr));

    LOG_INFO("Order cancelled: " + std::to_string(order_id));

    OrderUpdate update;
    update.order_id = order_id;
    update.update_type = OrderUpdate::UpdateType::Cancel;
    update.quantity_filled = order->quantityFilled();
    update.avg_price = order->avgPrice();
    update.new_status = order->status();
    update.message = "Order cancelled";
    notifyUpdate(update);

    return true;
}

framework::Status OrderService::modifyOrder(int64_t order_id,
                                               int64_t new_quantity,
                                               double new_price) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!doModifyOrder(order_id, new_quantity, new_price)) {
        return framework::Status::NotFound("Order not found: " + std::to_string(order_id));
    }

    return framework::Status::OK();
}

bool OrderService::doModifyOrder(int64_t order_id,
                                   int64_t new_quantity,
                                   double new_price) {
    auto it = orders_by_id_.find(order_id);
    if (it == orders_by_id_.end()) {
        return false;
    }

    auto order = it->second;
    if (order->isTerminal()) {
        return false;
    }

    order->setQuantity(new_quantity);
    order->setPrice(new_price);
    order->setUpdateTime(std::time(nullptr));

    LOG_INFO("Order modified: " + std::to_string(order_id));

    OrderUpdate update;
    update.order_id = order_id;
    update.update_type = OrderUpdate::UpdateType::Modify;
    update.quantity_filled = order->quantityFilled();
    update.avg_price = order->avgPrice();
    update.new_status = order->status();
    update.message = "Order modified";
    notifyUpdate(update);

    return true;
}

std::shared_ptr<Order> OrderService::getOrder(int64_t order_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = orders_by_id_.find(order_id);
    if (it != orders_by_id_.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<Order> OrderService::getOrderByClOrdId(const std::string& cl_ord_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = orders_by_cl_ord_id_.find(cl_ord_id);
    if (it != orders_by_cl_ord_id_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Order>> OrderService::getOrdersByAccount(int64_t account_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::shared_ptr<Order>> result;
    for (const auto& kv : orders_by_id_) {
        if (kv.second->accountId() == account_id) {
            result.push_back(kv.second);
        }
    }
    return result;
}

std::vector<std::shared_ptr<Order>> OrderService::getOrdersByStatus(OrderStatus status) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::shared_ptr<Order>> result;
    for (const auto& kv : orders_by_id_) {
        if (kv.second->status() == status) {
            result.push_back(kv.second);
        }
    }
    return result;
}

void OrderService::onOrderFill(int64_t order_id, int64_t fill_quantity, double fill_price) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = orders_by_id_.find(order_id);
    if (it == orders_by_id_.end()) {
        return;
    }

    auto order = it->second;
    int64_t new_filled = order->quantityFilled() + fill_quantity;
    order->setQuantityFilled(new_filled);

    // Calculate average price
    double total_value = order->avgPrice() * order->quantityFilled() +
                         fill_price * fill_quantity;
    order->setAvgPrice(total_value / new_filled);

    order->setUpdateTime(std::time(nullptr));

    if (new_filled >= order->quantity()) {
        order->setStatus(OrderStatus::Filled);
    } else {
        order->setStatus(OrderStatus::PartiallyFilled);
    }

    LOG_INFO("Order filled: " + std::to_string(order_id) +
             ", filled: " + std::to_string(new_filled) +
             ", price: " + std::to_string(fill_price));

    OrderUpdate update;
    update.order_id = order_id;
    update.update_type = OrderUpdate::UpdateType::Fill;
    update.quantity_filled = new_filled;
    update.avg_price = order->avgPrice();
    update.new_status = order->status();
    update.message = "Order filled";
    notifyUpdate(update);
}

void OrderService::onOrderReject(int64_t order_id, const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = orders_by_id_.find(order_id);
    if (it == orders_by_id_.end()) {
        return;
    }

    auto order = it->second;
    order->setStatus(OrderStatus::Rejected);
    order->setRejectReason(reason);
    order->setUpdateTime(std::time(nullptr));

    LOG_WARNING("Order rejected: " + std::to_string(order_id) + ", reason: " + reason);

    OrderUpdate update;
    update.order_id = order_id;
    update.update_type = OrderUpdate::UpdateType::Reject;
    update.quantity_filled = order->quantityFilled();
    update.avg_price = order->avgPrice();
    update.new_status = order->status();
    update.message = reason;
    notifyUpdate(update);
}

void OrderService::notifyUpdate(const OrderUpdate& update) {
    if (listener_) {
        listener_(update);
    }
}

size_t OrderService::totalOrders() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return orders_by_id_.size();
}

size_t OrderService::pendingOrders() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& kv : orders_by_id_) {
        if (kv.second->status() == OrderStatus::Pending ||
            kv.second->status() == OrderStatus::New) {
            count++;
        }
    }
    return count;
}

size_t OrderService::activeOrders() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& kv : orders_by_id_) {
        if (!kv.second->isTerminal()) {
            count++;
        }
    }
    return count;
}

// InMemoryOrderRepository implementation
bool InMemoryOrderRepository::save(const Order& order) {
    std::lock_guard<std::mutex> lock(mutex_);
    orders_[order.orderId()] = order;
    return true;
}

bool InMemoryOrderRepository::update(const Order& order) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = orders_.find(order.orderId());
    if (it != orders_.end()) {
        it->second = order;
        return true;
    }
    return false;
}

bool InMemoryOrderRepository::remove(int64_t order_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return orders_.erase(order_id) > 0;
}

std::unique_ptr<Order> InMemoryOrderRepository::findById(int64_t order_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = orders_.find(order_id);
    if (it != orders_.end()) {
        return std::make_unique<Order>(it->second);
    }
    return nullptr;
}

std::vector<Order> InMemoryOrderRepository::findByAccount(int64_t account_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Order> result;
    for (const auto& kv : orders_) {
        if (kv.second.accountId() == account_id) {
            result.push_back(kv.second);
        }
    }
    return result;
}

std::vector<Order> InMemoryOrderRepository::findByStatus(OrderStatus status) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Order> result;
    for (const auto& kv : orders_) {
        if (kv.second.status() == status) {
            result.push_back(kv.second);
        }
    }
    return result;
}

} // namespace order
} // namespace tradex