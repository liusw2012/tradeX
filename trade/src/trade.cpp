#include "tradex/trade/trade.h"
#include "tradex/framework/log.h"
#include <sstream>
#include <iomanip>
#include <ctime>

namespace tradex {
namespace trade {

Trade::Trade(int64_t trade_id, int64_t order_id, int64_t account_id,
             const std::string& symbol, const std::string& cl_ord_id,
             order::OrderSide side, int64_t quantity, double price)
    : trade_id_(trade_id), order_id_(order_id), account_id_(account_id),
      symbol_(symbol), cl_ord_id_(cl_ord_id), side_(side),
      quantity_(quantity), price_(price), commission_(0.0),
      trade_time_(std::time(nullptr)) {}

std::string Trade::toString() const {
    std::ostringstream oss;
    oss << "Trade{id=" << trade_id_
        << ", order=" << order_id_
        << ", symbol=" << symbol_
        << ", side=" << (side_ == order::OrderSide::Buy ? "BUY" : "SELL")
        << ", qty=" << quantity_
        << ", price=" << std::fixed << std::setprecision(2) << price_
        << ", commission=" << commission_
        << "}";
    return oss.str();
}

Position::Position(int64_t account_id, const std::string& symbol)
    : account_id_(account_id), symbol_(symbol),
      quantity_(0), avg_cost_(0.0),
      today_buy_(0), today_sell_(0), frozen_(0) {}

void Position::addBuy(int64_t qty, double price) {
    if (qty <= 0) return;

    double total_cost = avg_cost_ * quantity_ + price * qty;
    quantity_ += qty;
    today_buy_ += qty;

    if (quantity_ > 0) {
        avg_cost_ = total_cost / quantity_;
    }
}

void Position::addSell(int64_t qty, double price) {
    if (qty <= 0) return;

    quantity_ -= qty;
    today_sell_ += qty;

    // Reset average cost if position closed
    if (quantity_ == 0) {
        avg_cost_ = 0.0;
    }
}

void Position::update(const Trade& trade) {
    if (trade.side() == order::OrderSide::Buy) {
        addBuy(trade.quantity(), trade.price());
    } else {
        addSell(trade.quantity(), trade.price());
    }
}

Asset::Asset(int64_t account_id)
    : account_id_(account_id), cash_(0.0), frozen_cash_(0.0),
      total_market_value_(0.0), total_asset_(0.0) {}

void Asset::updateCash(double amount) {
    cash_ += amount;
    total_asset_ = cash_ + total_market_value_;
}

void Asset::freezeCash(double amount) {
    if (amount > availableCash()) {
        throw std::runtime_error("Insufficient available cash");
    }
    frozen_cash_ += amount;
}

void Asset::unfreezeCash(double amount) {
    frozen_cash_ = std::max(0.0, frozen_cash_ - amount);
}

void Asset::updateMarketValue(double value) {
    total_market_value_ = value;
    total_asset_ = cash_ + total_market_value_;
}

// TradeService implementation
TradeService::TradeService() : next_trade_id_(1000000) {}

bool TradeService::initialize() {
    LOG_INFO("TradeService initialized");
    return true;
}

void TradeService::executeTrade(const order::Order& order,
                                 int64_t fill_quantity,
                                 double fill_price) {
    // Create trade record
    auto trade = std::make_shared<Trade>(
        next_trade_id_++,
        order.orderId(),
        order.accountId(),
        order.symbol(),
        order.clOrdId(),
        order.side(),
        fill_quantity,
        fill_price
    );

    // Calculate commission (0.0003 for A-shares, min 5 RMB)
    double commission = std::max(5.0, fill_quantity * fill_price * 0.0003);
    trade->setCommission(commission);

    // Store trade
    {
        std::lock_guard<std::mutex> lock(mutex_);
        trades_by_order_[order.orderId()].push_back(trade);
    }

    // Update position
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::pair<int64_t, std::string> key = {order.accountId(), order.symbol()};
        auto it = positions_.find(key);

        if (it == positions_.end()) {
            auto pos = std::make_shared<Position>(order.accountId(), order.symbol());
            pos->update(*trade);
            positions_[key] = pos;
        } else {
            it->second->update(*trade);
        }
    }

    // Update asset
    {
        std::lock_guard<std::mutex> lock(mutex_);
        double trade_amount = fill_quantity * fill_price;
        double commission = trade->commission();

        auto it = assets_.find(order.accountId());
        if (it == assets_.end()) {
            auto asset = std::make_shared<Asset>(order.accountId());
            if (order.side() == order::OrderSide::Buy) {
                asset->updateCash(-trade_amount - commission);
            } else {
                asset->updateCash(trade_amount - commission);
            }
            assets_[order.accountId()] = asset;
        } else {
            if (order.side() == order::OrderSide::Buy) {
                it->second->updateCash(-trade_amount - commission);
            } else {
                it->second->updateCash(trade_amount - commission);
            }
        }
    }

    LOG_INFO("Trade executed: " + trade->toString());
}

void TradeService::onOrderFill(int64_t order_id, int64_t fill_quantity, double fill_price) {
    // This would be called by order service - placeholder
    // In real implementation, would fetch order and call executeTrade
}

std::shared_ptr<Position> TradeService::getPosition(int64_t account_id, const std::string& symbol) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::pair<int64_t, std::string> key = {account_id, symbol};
    auto it = positions_.find(key);
    if (it != positions_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Position>> TradeService::getPositions(int64_t account_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<Position>> result;
    for (const auto& kv : positions_) {
        if (kv.first.first == account_id) {
            result.push_back(kv.second);
        }
    }
    return result;
}

std::shared_ptr<Asset> TradeService::getAsset(int64_t account_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = assets_.find(account_id);
    if (it != assets_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Trade>> TradeService::getTrades(int64_t account_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<Trade>> result;
    for (const auto& kv : trades_by_order_) {
        for (const auto& trade : kv.second) {
            if (trade->accountId() == account_id) {
                result.push_back(trade);
            }
        }
    }
    return result;
}

std::vector<std::shared_ptr<Trade>> TradeService::getTradesByOrder(int64_t order_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = trades_by_order_.find(order_id);
    if (it != trades_by_order_.end()) {
        return it->second;
    }
    return {};
}

double TradeService::getTotalMarketValue(int64_t account_id) const {
    // Placeholder - would calculate from positions and market prices
    return 0.0;
}

double TradeService::getTotalAsset(int64_t account_id) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    auto it = assets_.find(account_id);
    if (it != assets_.end()) {
        return it->second->totalAsset();
    }
    return 0.0;
}

} // namespace trade
} // namespace tradex