#include "tradex/risk/risk.h"
#include "tradex/framework/log.h"
#include <algorithm>

namespace tradex {
namespace risk {

RiskManager::RiskManager() {}

bool RiskManager::initialize() {
    // Add default account configuration for testing
    RiskConfig default_config;
    default_config.account_id = 12345;
    default_config.max_position_value = 1000000;
    default_config.max_single_order_qty = 100000;
    default_config.max_single_order_amount = 1000000;
    default_config.max_daily_buy = 10000000;
    default_config.max_daily_sell = 10000000;
    default_config.max_margin_ratio = 1.0;
    default_config.max_total_position = 50000000;

    setRiskConfig(12345, default_config);

    LOG_INFO("RiskManager initialized with default account 12345");
    return true;
}

void RiskManager::setRiskConfig(int64_t account_id, const RiskConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    configs_[account_id] = config;
}

RiskConfig RiskManager::getRiskConfig(int64_t account_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = configs_.find(account_id);
    if (it != configs_.end()) {
        return it->second;
    }
    return RiskConfig{account_id};
}

std::vector<RiskCheckResult> RiskManager::checkOrder(const order::Order& order) {
    std::vector<RiskCheckResult> results;

    // Check account exists
    auto account_check = checkAccountExists(order.accountId());
    if (!account_check.passed) {
        results.push_back(account_check);
        return results;
    }

    // Check order limit
    auto order_limit = checkSingleOrderLimit(order);
    if (!order_limit.passed) {
        results.push_back(order_limit);
    }

    // Check daily buy/sell limit
    double order_amount = order.price() * order.quantity();
    if (order.side() == order::OrderSide::Buy) {
        auto daily_buy = checkDailyBuyLimit(order.accountId(),
                                             static_cast<int64_t>(order_amount));
        if (!daily_buy.passed) {
            results.push_back(daily_buy);
        }
    } else {
        auto daily_sell = checkDailySellLimit(order.accountId(),
                                                static_cast<int64_t>(order_amount));
        if (!daily_sell.passed) {
            results.push_back(daily_sell);
        }
    }

    // Check position limit
    auto position_limit = checkPositionLimit(order.accountId(),
                                               order.symbol(),
                                               order.quantity());
    if (!position_limit.passed) {
        results.push_back(position_limit);
    }

    // Check margin for buy orders
    if (order.side() == order::OrderSide::Buy) {
        auto margin = checkMargin(order.accountId(), order_amount);
        if (!margin.passed) {
            results.push_back(margin);
        }
    }

    // Check price limit
    auto price_limit = checkPriceLimit(order.symbol(), order.price(), order.side());
    if (!price_limit.passed) {
        results.push_back(price_limit);
    }

    return results;
}

RiskCheckResult RiskManager::checkAccountExists(int64_t account_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (configs_.find(account_id) == configs_.end()) {
        return RiskCheckResult::fail(RiskRuleType::AccountMargin,
                                      "AccountCheck",
                                      "Account not found: " + std::to_string(account_id));
    }
    return RiskCheckResult::pass();
}

RiskCheckResult RiskManager::checkMargin(int64_t account_id, double required_amount) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = configs_.find(account_id);
    if (it == configs_.end()) {
        return RiskCheckResult::pass();  // No config, allow
    }

    // In real implementation, would check actual account balance
    // Placeholder: assume sufficient margin
    return RiskCheckResult::pass();
}

RiskCheckResult RiskManager::checkPositionLimit(int64_t account_id,
                                                  const std::string& symbol,
                                                  int64_t quantity) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = configs_.find(account_id);
    if (it == configs_.end()) {
        return RiskCheckResult::pass();
    }

    const auto& config = it->second;
    if (quantity > config.max_single_order_qty) {
        return RiskCheckResult::fail(
            RiskRuleType::PositionLimit,
            "PositionLimit",
            "Position quantity exceeds limit",
            config.max_single_order_qty,
            quantity
        );
    }

    return RiskCheckResult::pass();
}

RiskCheckResult RiskManager::checkDailyBuyLimit(int64_t account_id, int64_t amount) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto config_it = configs_.find(account_id);
    if (config_it == configs_.end()) {
        return RiskCheckResult::pass();
    }

    auto stats_it = daily_stats_.find(account_id);
    int64_t current_buy = 0;
    if (stats_it != daily_stats_.end()) {
        current_buy = stats_it->second.total_buy;
    }

    if (current_buy + amount > config_it->second.max_daily_buy) {
        return RiskCheckResult::fail(
            RiskRuleType::DailyTradeLimit,
            "DailyBuyLimit",
            "Daily buy limit exceeded",
            config_it->second.max_daily_buy,
            current_buy + amount
        );
    }

    return RiskCheckResult::pass();
}

RiskCheckResult RiskManager::checkDailySellLimit(int64_t account_id, int64_t amount) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto config_it = configs_.find(account_id);
    if (config_it == configs_.end()) {
        return RiskCheckResult::pass();
    }

    auto stats_it = daily_stats_.find(account_id);
    int64_t current_sell = 0;
    if (stats_it != daily_stats_.end()) {
        current_sell = stats_it->second.total_sell;
    }

    if (current_sell + amount > config_it->second.max_daily_sell) {
        return RiskCheckResult::fail(
            RiskRuleType::DailyTradeLimit,
            "DailySellLimit",
            "Daily sell limit exceeded",
            config_it->second.max_daily_sell,
            current_sell + amount
        );
    }

    return RiskCheckResult::pass();
}

RiskCheckResult RiskManager::checkSingleOrderLimit(const order::Order& order) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = configs_.find(order.accountId());
    if (it == configs_.end()) {
        return RiskCheckResult::pass();
    }

    const auto& config = it->second;
    double order_amount = order.price() * order.quantity();

    if (order.quantity() > config.max_single_order_qty) {
        return RiskCheckResult::fail(
            RiskRuleType::SingleOrderLimit,
            "SingleOrderQtyLimit",
            "Single order quantity exceeds limit",
            config.max_single_order_qty,
            order.quantity()
        );
    }

    if (order_amount > config.max_single_order_amount) {
        return RiskCheckResult::fail(
            RiskRuleType::SingleOrderLimit,
            "SingleOrderAmountLimit",
            "Single order amount exceeds limit",
            static_cast<int64_t>(config.max_single_order_amount),
            static_cast<int64_t>(order_amount)
        );
    }

    return RiskCheckResult::pass();
}

RiskCheckResult RiskManager::checkPriceLimit(const std::string& symbol,
                                                double price,
                                                order::OrderSide side) {
    // Placeholder - in real implementation would check against
    // daily price limit from market data
    return RiskCheckResult::pass();
}

void RiskManager::recordTrade(int64_t account_id, int64_t amount, bool is_buy) {
    std::lock_guard<std::mutex> lock(mutex_);
    daily_stats_[account_id].trade_count++;
    if (is_buy) {
        daily_stats_[account_id].total_buy += amount;
    } else {
        daily_stats_[account_id].total_sell += amount;
    }
}

void RiskManager::resetDailyStats(int64_t account_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    daily_stats_[account_id] = DailyStats();
}

// Risk rule implementations
RiskCheckResult MarginCheckRule::check(const order::Order& order) {
    if (order.side() != order::OrderSide::Buy) {
        return RiskCheckResult::pass();
    }

    double required = order.price() * order.quantity();
    return manager_->checkMargin(order.accountId(), required);
}

RiskCheckResult PositionLimitRule::check(const order::Order& order) {
    return manager_->checkPositionLimit(order.accountId(), order.symbol(), order.quantity());
}

RiskCheckResult OrderLimitRule::check(const order::Order& order) {
    return manager_->checkSingleOrderLimit(order);
}

RiskCheckResult PriceLimitRule::check(const order::Order& order) {
    return manager_->checkPriceLimit(order.symbol(), order.price(), order.side());
}

} // namespace risk
} // namespace tradex