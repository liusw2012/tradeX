// Test order functionality
#include "tradex/order/order.h"
#include "tradex/framework/status.h"
#include "test_framework.h"

// Test order creation
void test_order_creation() {
    using namespace tradex::order;

    Order order;
    order.setOrderId(12345);
    order.setAccountId(100);
    order.setSymbol("600000");
    order.setSide(OrderSide::Buy);
    order.setType(OrderType::Limit);
    order.setQuantity(100);
    order.setPrice(10.50);
    order.setStatus(OrderStatus::New);

    ASSERT_EQ(order.orderId(), 12345);
    ASSERT_EQ(order.accountId(), 100);
    ASSERT_EQ(order.symbol(), "600000");
    ASSERT_EQ(order.side(), OrderSide::Buy);
    ASSERT_EQ(order.type(), OrderType::Limit);
    ASSERT_EQ(order.quantity(), 100);
    ASSERT_EQ(order.price(), 10.50);
    ASSERT_EQ(order.status(), OrderStatus::New);
}

// Test order status transitions
void test_order_status() {
    using namespace tradex::order;

    Order order;
    order.setStatus(OrderStatus::New);
    ASSERT_EQ(order.status(), OrderStatus::New);

    order.setStatus(OrderStatus::Pending);
    ASSERT_EQ(order.status(), OrderStatus::Pending);

    order.setStatus(OrderStatus::Filled);
    ASSERT_EQ(order.status(), OrderStatus::Filled);
}