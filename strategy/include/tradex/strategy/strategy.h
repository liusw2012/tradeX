#pragma once

#include "tradex/order/order.h"
#include "tradex/market/market.h"
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <mutex>

namespace tradex {
namespace strategy {

// Strategy type
enum class StrategyType {
    Manual = 0,
    TrendFollowing = 1,
    MeanReversion = 2,
    Grid = 3,
    Scalping = 4,
    Custom = 100
};

// Strategy state
enum class StrategyState {
    Stopped = 0,
    Running = 1,
    Paused = 2,
    Error = 3
};

// Strategy configuration
struct StrategyConfig {
    int64_t strategy_id;
    std::string name;
    StrategyType type;
    int64_t account_id;

    // Trading parameters
    std::string symbol;
    double max_position;       // Max position size
    double max_drawdown;       // Max drawdown percentage
    int64_t max_orders_per_day;

    // Risk parameters
    double stop_loss;          // Stop loss percentage
    double take_profit;        // Take profit percentage
    double position_size_pct;  // Position size as % of capital

    // Strategy specific parameters (JSON)
    std::string params_json;
};

// Strategy state record
struct StrategyStateRecord {
    int64_t strategy_id;
    StrategyState state;
    int64_t position;
    double unrealized_pnl;
    double realized_pnl;
    int64_t order_count_today;
    int64_t last_trade_time;
    std::string last_error;
};

// Strategy signal
struct StrategySignal {
    int64_t strategy_id;
    std::string symbol;
    order::OrderSide side;     // Buy or Sell
    int64_t quantity;
    double target_price;
    std::string reason;
    int64_t timestamp;
};

// Strategy engine interface
class IStrategyEngine {
public:
    virtual ~IStrategyEngine() = default;

    virtual int64_t createStrategy(const StrategyConfig& config) = 0;
    virtual bool deleteStrategy(int64_t strategy_id) = 0;

    virtual bool startStrategy(int64_t strategy_id) = 0;
    virtual bool stopStrategy(int64_t strategy_id) = 0;
    virtual bool pauseStrategy(int64_t strategy_id) = 0;
    virtual bool resumeStrategy(int64_t strategy_id) = 0;

    virtual std::shared_ptr<StrategyStateRecord> getStrategyState(int64_t strategy_id) = 0;
    virtual std::vector<std::shared_ptr<StrategyStateRecord>> getAllStrategyStates() = 0;
};

// Strategy base class
class Strategy {
public:
    virtual ~Strategy() = default;

    virtual void onTick(const market::Tick& tick) = 0;
    virtual void onOrderUpdate(const order::OrderUpdate& update) = 0;
    virtual void onBar(const std::string& symbol, int interval) = 0;

    virtual bool initialize() = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;

    int64_t id() const { return config_.strategy_id; }
    const std::string& name() const { return config_.name; }
    StrategyState state() const { return state_; }

protected:
    Strategy(const StrategyConfig& config) : config_(config), state_(StrategyState::Stopped) {}

    StrategyConfig config_;
    StrategyState state_;

    // Send order helper
    void sendSignal(const StrategySignal& signal);

    // Callbacks
    using SignalCallback = std::function<void(const StrategySignal&)>;
    SignalCallback signal_callback_;
};

// Moving average cross strategy
class MAStrategy : public Strategy {
public:
    MAStrategy(const StrategyConfig& config);

    bool initialize() override;
    bool start() override;
    bool stop() override;

    void onTick(const market::Tick& tick) override;
    void onOrderUpdate(const order::OrderUpdate& update) override;
    void onBar(const std::string& symbol, int interval) override;

private:
    void calculateIndicators();
    bool checkEntry();
    bool checkExit();

    int fast_ma_period_;
    int slow_ma_period_;
    double current_fast_ma_;
    double current_slow_ma_;
    int64_t position_;
};

// Grid strategy
class GridStrategy : public Strategy {
public:
    GridStrategy(const StrategyConfig& config);

    bool initialize() override;
    bool start() override;
    bool stop() override;

    void onTick(const market::Tick& tick) override;
    void onOrderUpdate(const order::OrderUpdate& update) override;
    void onBar(const std::string& symbol, int interval) override;

private:
    void placeGridOrders();
    void checkGridLevels();

    double grid_spacing_;
    int grid_levels_;
    double base_price_;
    std::vector<int64_t> grid_orders_;
};

} // namespace strategy
} // namespace tradex