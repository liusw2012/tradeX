#pragma once

#include "tradex/order/order.h"
#include "tradex/trade/trade.h"
#include "tradex/framework/status.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>

namespace tradex {
namespace risk {

// Risk rule type
enum class RiskRuleType {
    AccountMargin = 1,      // Account margin check
    PositionLimit = 2,      // Position limit
    SingleOrderLimit = 3,   // Single order limit
    DailyTradeLimit = 4,    // Daily trading limit
    PriceLimit = 5,        // Price limit (涨跌停)
    StockStatus = 6,       // Stock trading status
    SymbolFilter = 7,      // Symbol filter
    AmountLimit = 8        // Amount limit
};

// Risk check result
struct RiskCheckResult {
    bool passed;
    RiskRuleType rule_type;
    std::string rule_name;
    std::string message;
    int64_t limit_value;
    int64_t current_value;

    static RiskCheckResult pass() {
        return {true, RiskRuleType::AccountMargin, "", "", 0, 0};
    }

    static RiskCheckResult fail(RiskRuleType type, const std::string& name,
                                 const std::string& msg, int64_t limit = 0, int64_t current = 0) {
        return {false, type, name, msg, limit, current};
    }
};

// Risk configuration
struct RiskConfig {
    int64_t account_id;

    // Account level limits
    double max_margin_ratio = 0.8;         // Maximum margin ratio
    double min_maintenance_margin = 3000;  // Minimum maintenance margin (RMB)
    double max_daily_loss = 100000;        // Maximum daily loss

    // Position limits
    int64_t max_position_value = 10000000; // Maximum position value per stock
    int64_t max_total_position = 50000000; // Maximum total position

    // Order limits
    int64_t max_single_order_qty = 100000; // Maximum single order quantity
    double max_single_order_amount = 1000000; // Maximum single order amount

    // Daily limits
    int64_t max_daily_buy = 20000000;      // Maximum daily buy
    int64_t max_daily_sell = 20000000;    // Maximum daily sell
    int64_t max_daily_trades = 1000;       // Maximum daily trades
};

// Risk manager
class RiskManager {
public:
    RiskManager();
    ~RiskManager() = default;

    bool initialize();

    // Set risk config for account
    void setRiskConfig(int64_t account_id, const RiskConfig& config);

    // Check order risk
    std::vector<RiskCheckResult> checkOrder(const order::Order& order);

    // Check margin sufficiency
    RiskCheckResult checkMargin(int64_t account_id, double required_amount);

    // Check position limit
    RiskCheckResult checkPositionLimit(int64_t account_id,
                                        const std::string& symbol,
                                        int64_t quantity);

    // Check daily limit
    RiskCheckResult checkDailyBuyLimit(int64_t account_id, int64_t amount);
    RiskCheckResult checkDailySellLimit(int64_t account_id, int64_t amount);

    // Check single order limit
    RiskCheckResult checkSingleOrderLimit(const order::Order& order);

    // Check price limit (涨跌停)
    RiskCheckResult checkPriceLimit(const std::string& symbol, double price, order::OrderSide side);

    // Update daily statistics
    void recordTrade(int64_t account_id, int64_t amount, bool is_buy);

    // Reset daily statistics
    void resetDailyStats(int64_t account_id);

    // Get risk config
    RiskConfig getRiskConfig(int64_t account_id);

private:
    RiskCheckResult checkAccountExists(int64_t account_id);

    mutable std::mutex mutex_;

    // Risk configs by account
    std::unordered_map<int64_t, RiskConfig> configs_;

    // Daily statistics
    struct DailyStats {
        int64_t total_buy = 0;
        int64_t total_sell = 0;
        int64_t trade_count = 0;
        double daily_pnl = 0.0;
    };

    std::unordered_map<int64_t, DailyStats> daily_stats_;
};

// Risk rule interface (Strategy pattern)
class IRiskRule {
public:
    virtual ~IRiskRule() = default;
    virtual RiskCheckResult check(const order::Order& order) = 0;
    virtual std::string name() const = 0;
};

// Margin check rule
class MarginCheckRule : public IRiskRule {
public:
    MarginCheckRule(RiskManager* manager) : manager_(manager) {}

    RiskCheckResult check(const order::Order& order) override;
    std::string name() const override { return "MarginCheck"; }

private:
    RiskManager* manager_;
};

// Position limit rule
class PositionLimitRule : public IRiskRule {
public:
    PositionLimitRule(RiskManager* manager) : manager_(manager) {}

    RiskCheckResult check(const order::Order& order) override;
    std::string name() const override { return "PositionLimit"; }

private:
    RiskManager* manager_;
};

// Order limit rule
class OrderLimitRule : public IRiskRule {
public:
    OrderLimitRule(RiskManager* manager) : manager_(manager) {}

    RiskCheckResult check(const order::Order& order) override;
    std::string name() const override { return "OrderLimit"; }

private:
    RiskManager* manager_;
};

// Price limit rule
class PriceLimitRule : public IRiskRule {
public:
    explicit PriceLimitRule(RiskManager* manager = nullptr) : manager_(manager) {}

    RiskCheckResult check(const order::Order& order) override;
    std::string name() const override { return "PriceLimit"; }

    void setLimitRatio(double ratio) { limit_ratio_ = ratio; }
    void setManager(RiskManager* manager) { manager_ = manager; }

private:
    double limit_ratio_ = 0.1;  // 10% for A-shares
    RiskManager* manager_ = nullptr;
};

} // namespace risk
} // namespace tradex