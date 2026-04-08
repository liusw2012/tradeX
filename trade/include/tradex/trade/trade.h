#pragma once

#include "tradex/order/order.h"
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>

namespace tradex {
namespace trade {

// Hash function for pair (must be defined before use)
struct pair_hash {
    size_t operator()(const std::pair<int64_t, std::string>& p) const {
        return std::hash<int64_t>()(p.first) ^ std::hash<std::string>()(p.second);
    }
};

// Trade execution record
class Trade {
public:
    Trade() : trade_id_(0), order_id_(0), account_id_(0),
              quantity_(0), price_(0.0), commission_(0.0) {}

    Trade(int64_t trade_id, int64_t order_id, int64_t account_id,
          const std::string& symbol, const std::string& cl_ord_id,
          order::OrderSide side, int64_t quantity, double price);

    // Getters
    int64_t tradeId() const { return trade_id_; }
    int64_t orderId() const { return order_id_; }
    int64_t accountId() const { return account_id_; }
    const std::string& symbol() const { return symbol_; }
    const std::string& clOrdId() const { return cl_ord_id_; }
    order::OrderSide side() const { return side_; }
    int64_t quantity() const { return quantity_; }
    double price() const { return price_; }
    double commission() const { return commission_; }
    int64_t tradeTime() const { return trade_time_; }

    // Setters
    void setTradeId(int64_t id) { trade_id_ = id; }
    void setCommission(double comm) { commission_ = comm; }
    void setTradeTime(int64_t time) { trade_time_ = time; }

    std::string toString() const;

private:
    int64_t trade_id_;
    int64_t order_id_;
    int64_t account_id_;
    std::string symbol_;
    std::string cl_ord_id_;
    order::OrderSide side_;
    int64_t quantity_;
    double price_;
    double commission_;
    int64_t trade_time_;
};

// Position record
class Position {
public:
    Position() : account_id_(0), symbol_(),
                 quantity_(0), avg_cost_(0.0),
                 today_buy_(0), today_sell_(0),
                 frozen_(0) {}

    Position(int64_t account_id, const std::string& symbol);

    // Getters
    int64_t accountId() const { return account_id_; }
    const std::string& symbol() const { return symbol_; }
    int64_t quantity() const { return quantity_; }
    double avgCost() const { return avg_cost_; }
    int64_t todayBuy() const { return today_buy_; }
    int64_t todaySell() const { return today_sell_; }
    int64_t frozen() const { return frozen_; }

    // Market value
    double marketValue(double current_price) const {
        return quantity_ * current_price;
    }

    // Unrealized P&L
    double unrealizedPnL(double current_price) const {
        return (current_price - avg_cost_) * quantity_;
    }

    // Setters
    void setQuantity(int64_t qty) { quantity_ = qty; }
    void setAvgCost(double cost) { avg_cost_ = cost; }
    void addBuy(int64_t qty, double price);
    void addSell(int64_t qty, double price);
    void setFrozen(int64_t f) { frozen_ = f; }

    // Update position
    void update(const Trade& trade);

private:
    int64_t account_id_;
    std::string symbol_;
    int64_t quantity_;      // Current position
    double avg_cost_;       // Average cost
    int64_t today_buy_;     // Today's buy volume
    int64_t today_sell_;    // Today's sell volume
    int64_t frozen_;        // Frozen quantity
};

// Account asset
class Asset {
public:
    Asset() : account_id_(0), cash_(0.0), frozen_cash_(0.0),
              total_market_value_(0.0), total_asset_(0.0) {}

    Asset(int64_t account_id);

    // Getters
    int64_t accountId() const { return account_id_; }
    double cash() const { return cash_; }
    double frozenCash() const { return frozen_cash_; }
    double availableCash() const { return cash_ - frozen_cash_; }
    double totalMarketValue() const { return total_market_value_; }
    double totalAsset() const { return total_asset_; }

    // Update
    void updateCash(double amount);
    void freezeCash(double amount);
    void unfreezeCash(double amount);
    void updateMarketValue(double value);

private:
    int64_t account_id_;
    double cash_;
    double frozen_cash_;
    double total_market_value_;
    double total_asset_;
};

// Trade service - handles trade execution
class TradeService {
public:
    TradeService();
    ~TradeService() = default;

    bool initialize();

    // Trade execution
    void onOrderFill(int64_t order_id, int64_t fill_quantity, double fill_price);

    // Position management
    std::shared_ptr<Position> getPosition(int64_t account_id, const std::string& symbol);
    std::vector<std::shared_ptr<Position>> getPositions(int64_t account_id);

    // Asset management
    std::shared_ptr<Asset> getAsset(int64_t account_id);

    // Trade history
    std::vector<std::shared_ptr<Trade>> getTrades(int64_t account_id);
    std::vector<std::shared_ptr<Trade>> getTradesByOrder(int64_t order_id);

    // Statistics
    double getTotalMarketValue(int64_t account_id) const;
    double getTotalAsset(int64_t account_id) const;

private:
    void executeTrade(const order::Order& order, int64_t fill_quantity, double fill_price);

    mutable std::mutex mutex_;

    // Trades by order
    std::unordered_map<int64_t, std::vector<std::shared_ptr<Trade>>> trades_by_order_;

    // Positions: (account_id, symbol) -> Position
    std::unordered_map<std::pair<int64_t, std::string>, std::shared_ptr<Position>,
                      pair_hash> positions_;

    // Assets: account_id -> Asset
    std::unordered_map<int64_t, std::shared_ptr<Asset>> assets_;

    int64_t next_trade_id_;
};

} // namespace trade
} // namespace tradex