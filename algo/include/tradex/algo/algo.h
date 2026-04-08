#pragma once

#include "tradex/order/order.h"
#include "tradex/order/order_service.h"
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <atomic>

namespace tradex {
namespace algo {

// Algo order type
enum class AlgoType {
    VWAP = 1,          // Volume Weighted Average Price
    TWAP = 2,          // Time Weighted Average Price
    POV = 3,           // Participation of Volume
    IS = 4,            // Implementation Shortfall
    Iceberg = 5,      // Iceberg order
    Ghost = 6,        // Ghost iceberg
    Stochastic = 7   // Stochastic
};

// Algo parameters
struct AlgoParams {
    // Common parameters
    AlgoType type;
    int64_t total_quantity;
    double limit_price;         // 0 for market
    int64_t start_time;         // Unix timestamp
    int64_t end_time;          // Unix timestamp

    // VWAP parameters
    double participation_rate = 0.1;  // 10% participation

    // TWAP parameters
    int num_slices = 10;
    int64_t slice_interval_sec = 60;

    // POV parameters
    double target_pov = 0.1;

    // Iceberg parameters
    int64_t display_size = 1000;
    int64_t max_visible_rate = 0.2;

    // Risk management
    bool allow_market = false;
    double max_slippage = 0.01;
};

// Algo order state
struct AlgoState {
    int64_t algo_order_id;
    int64_t parent_order_id;   // Child order ID

    AlgoType algo_type;
    std::string symbol;
    int64_t total_quantity;
    int64_t filled_quantity;
    double avg_fill_price;

    int64_t num_child_orders;
    int64_t num_child_filled;

    order::OrderStatus status;
    int64_t create_time;
    int64_t update_time;

    std::string message;
};

// Algo engine interface
class IAlgoEngine {
public:
    virtual ~IAlgoEngine() = default;

    virtual int64_t startAlgo(const AlgoParams& params) = 0;
    virtual bool stopAlgo(int64_t algo_order_id) = 0;
    virtual bool pauseAlgo(int64_t algo_order_id) = 0;
    virtual bool resumeAlgo(int64_t algo_order_id) = 0;

    virtual std::shared_ptr<AlgoState> getAlgoState(int64_t algo_order_id) = 0;
    virtual std::vector<std::shared_ptr<AlgoState>> getAllAlgoStates() = 0;
};

// Algo engine implementation
class AlgoEngine : public IAlgoEngine {
public:
    AlgoEngine();
    ~AlgoEngine() override;

    bool initialize();

    // Get order service
    void setOrderService(order::IOrderService* service) { order_service_ = service; }

    // Algo lifecycle
    int64_t startAlgo(const AlgoParams& params) override;
    bool stopAlgo(int64_t algo_order_id) override;
    bool pauseAlgo(int64_t algo_order_id) override;
    bool resumeAlgo(int64_t algo_order_id) override;

    // Query
    std::shared_ptr<AlgoState> getAlgoState(int64_t algo_order_id) override;
    std::vector<std::shared_ptr<AlgoState>> getAllAlgoStates() override;

    // Callbacks
    using FillCallback = std::function<void(int64_t, int64_t, double)>;
    void setFillCallback(FillCallback cb) { fill_cb_ = cb; }

private:
    void executeSlice(int64_t algo_order_id);

    order::IOrderService* order_service_;
    mutable std::mutex mutex_;

    std::unordered_map<int64_t, std::shared_ptr<AlgoState>> algos_;
    std::atomic<int64_t> next_algo_id_;
    FillCallback fill_cb_;
};

// Base algo strategy
class AlgoStrategy {
public:
    virtual ~AlgoStrategy() = default;

    virtual void onFill(int64_t filled_qty, double fill_price) = 0;
    virtual int64_t calculateNextSlice() = 0;
    virtual bool shouldContinue() = 0;

    std::string name() const { return name_; }

protected:
    AlgoStrategy(const std::string& name, const AlgoParams& params)
        : name_(name), params_(params) {}

    AlgoParams params_;
    std::string name_;
};

// VWAP strategy
class VWAPStrategy : public AlgoStrategy {
public:
    VWAPStrategy(const AlgoParams& params);

    void onFill(int64_t filled_qty, double fill_price) override;
    int64_t calculateNextSlice() override;
    bool shouldContinue() override;

private:
    int64_t remaining_qty_;
    int current_slice_;
};

// TWAP strategy
class TWAPStrategy : public AlgoStrategy {
public:
    TWAPStrategy(const AlgoParams& params);

    void onFill(int64_t filled_qty, double fill_price) override;
    int64_t calculateNextSlice() override;
    bool shouldContinue() override;

private:
    int64_t remaining_qty_;
    int current_slice_;
};

// Iceberg strategy
class IcebergStrategy : public AlgoStrategy {
public:
    IcebergStrategy(const AlgoParams& params);

    void onFill(int64_t filled_qty, double fill_price) override;
    int64_t calculateNextSlice() override;
    bool shouldContinue() override;

private:
    int64_t remaining_qty_;
    int64_t display_qty_;
};

} // namespace algo
} // namespace tradex