#include "tradex/strategy/strategy.h"
#include "tradex/framework/log.h"

namespace tradex {
namespace strategy {

void Strategy::sendSignal(const StrategySignal& signal) {
    if (signal_callback_) {
        signal_callback_(signal);
    }
}

// MAStrategy implementation
MAStrategy::MAStrategy(const StrategyConfig& config)
    : Strategy(config), fast_ma_period_(5), slow_ma_period_(20),
      current_fast_ma_(0.0), current_slow_ma_(0.0), position_(0) {}

bool MAStrategy::initialize() {
    LOG_INFO("MA Strategy initialized: " + config_.name);
    return true;
}

bool MAStrategy::start() {
    state_ = StrategyState::Running;
    LOG_INFO("MA Strategy started: " + config_.name);
    return true;
}

bool MAStrategy::stop() {
    state_ = StrategyState::Stopped;
    LOG_INFO("MA Strategy stopped: " + config_.name);
    return true;
}

void MAStrategy::onTick(const market::Tick& tick) {
    if (state_ != StrategyState::Running) {
        return;
    }

    calculateIndicators();

    if (checkEntry()) {
        StrategySignal signal;
        signal.strategy_id = config_.strategy_id;
        signal.symbol = config_.symbol;
        signal.side = order::OrderSide::Buy;
        signal.quantity = 1000;
        signal.target_price = tick.price;
        signal.reason = "MA cross up";
        signal.timestamp = tick.timestamp;
        sendSignal(signal);
    }
}

void MAStrategy::onOrderUpdate(const order::OrderUpdate& update) {
    // Handle order fills
}

void MAStrategy::onBar(const std::string& symbol, int interval) {
    // Process bar data
}

void MAStrategy::calculateIndicators() {
    // Simplified - would calculate real MA
    current_fast_ma_ = 100.0;
    current_slow_ma_ = 99.5;
}

bool MAStrategy::checkEntry() {
    return current_fast_ma_ > current_slow_ma_;
}

bool MAStrategy::checkExit() {
    return current_fast_ma_ < current_slow_ma_;
}

// GridStrategy implementation
GridStrategy::GridStrategy(const StrategyConfig& config)
    : Strategy(config), grid_spacing_(0.01), grid_levels_(10), base_price_(100.0) {}

bool GridStrategy::initialize() {
    LOG_INFO("Grid Strategy initialized: " + config_.name);
    return true;
}

bool GridStrategy::start() {
    placeGridOrders();
    state_ = StrategyState::Running;
    LOG_INFO("Grid Strategy started: " + config_.name);
    return true;
}

bool GridStrategy::stop() {
    state_ = StrategyState::Stopped;
    LOG_INFO("Grid Strategy stopped: " + config_.name);
    return true;
}

void GridStrategy::onTick(const market::Tick& tick) {
    if (state_ != StrategyState::Running) {
        return;
    }

    checkGridLevels();
}

void GridStrategy::onOrderUpdate(const order::OrderUpdate& update) {
    // Handle order fills
}

void GridStrategy::onBar(const std::string& symbol, int interval) {
    // Process bar data
}

void GridStrategy::placeGridOrders() {
    grid_orders_.clear();
    base_price_ = 100.0;

    for (int i = 0; i < grid_levels_; ++i) {
        double buy_price = base_price_ - (i + 1) * grid_spacing_;
        double sell_price = base_price_ + (i + 1) * grid_spacing_;

        // Place grid orders
        grid_orders_.push_back(0);  // Order IDs would be stored here
    }
}

void GridStrategy::checkGridLevels() {
    // Check if price hit grid levels
}

} // namespace strategy
} // namespace tradex