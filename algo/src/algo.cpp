#include "tradex/algo/algo.h"
#include "tradex/framework/log.h"

namespace tradex {
namespace algo {

// AlgoEngine implementation
AlgoEngine::AlgoEngine() : order_service_(nullptr), next_algo_id_(1) {}

AlgoEngine::~AlgoEngine() {}

bool AlgoEngine::initialize() {
    LOG_INFO("AlgoEngine initialized");
    return true;
}

int64_t AlgoEngine::startAlgo(const AlgoParams& params) {
    int64_t algo_id = next_algo_id_++;

    auto state = std::make_shared<AlgoState>();
    state->algo_order_id = algo_id;
    state->algo_type = params.type;
    state->symbol = "";  // Set from order
    state->total_quantity = params.total_quantity;
    state->filled_quantity = 0;
    state->avg_fill_price = 0.0;
    state->num_child_orders = 0;
    state->num_child_filled = 0;
    state->status = order::OrderStatus::New;
    state->create_time = std::time(nullptr);
    state->update_time = std::time(nullptr);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        algos_[algo_id] = state;
    }

    LOG_INFO("Algo started: " + std::to_string(algo_id));
    return algo_id;
}

bool AlgoEngine::stopAlgo(int64_t algo_order_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = algos_.find(algo_order_id);
    if (it == algos_.end()) {
        return false;
    }

    it->second->status = order::OrderStatus::Cancelled;
    LOG_INFO("Algo stopped: " + std::to_string(algo_order_id));
    return true;
}

bool AlgoEngine::pauseAlgo(int64_t algo_order_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = algos_.find(algo_order_id);
    if (it == algos_.end()) {
        return false;
    }

    it->second->status = order::OrderStatus::Pending;
    LOG_INFO("Algo paused: " + std::to_string(algo_order_id));
    return true;
}

bool AlgoEngine::resumeAlgo(int64_t algo_order_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = algos_.find(algo_order_id);
    if (it == algos_.end()) {
        return false;
    }

    it->second->status = order::OrderStatus::New;
    LOG_INFO("Algo resumed: " + std::to_string(algo_order_id));
    return true;
}

std::shared_ptr<AlgoState> AlgoEngine::getAlgoState(int64_t algo_order_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = algos_.find(algo_order_id);
    if (it != algos_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<AlgoState>> AlgoEngine::getAllAlgoStates() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::shared_ptr<AlgoState>> result;
    for (const auto& kv : algos_) {
        result.push_back(kv.second);
    }
    return result;
}

void AlgoEngine::executeSlice(int64_t algo_order_id) {
    // Placeholder - would create child order and submit
}

// VWAPStrategy implementation
VWAPStrategy::VWAPStrategy(const AlgoParams& params)
    : AlgoStrategy("VWAP", params), remaining_qty_(params.total_quantity),
      current_slice_(0) {}

void VWAPStrategy::onFill(int64_t filled_qty, double fill_price) {
    remaining_qty_ -= filled_qty;
}

int64_t VWAPStrategy::calculateNextSlice() {
    int64_t slice_qty = params_.total_quantity / params_.num_slices;
    return std::min(slice_qty, remaining_qty_);
}

bool VWAPStrategy::shouldContinue() {
    return remaining_qty_ > 0;
}

// TWAPStrategy implementation
TWAPStrategy::TWAPStrategy(const AlgoParams& params)
    : AlgoStrategy("TWAP", params), remaining_qty_(params.total_quantity),
      current_slice_(0) {}

void TWAPStrategy::onFill(int64_t filled_qty, double fill_price) {
    remaining_qty_ -= filled_qty;
    current_slice_++;
}

int64_t TWAPStrategy::calculateNextSlice() {
    int64_t slice_qty = params_.total_quantity / params_.num_slices;
    return std::min(slice_qty, remaining_qty_);
}

bool TWAPStrategy::shouldContinue() {
    return remaining_qty_ > 0 && current_slice_ < params_.num_slices;
}

// IcebergStrategy implementation
IcebergStrategy::IcebergStrategy(const AlgoParams& params)
    : AlgoStrategy("Iceberg", params), remaining_qty_(params.total_quantity),
      display_qty_(params.display_size) {}

void IcebergStrategy::onFill(int64_t filled_qty, double fill_price) {
    remaining_qty_ -= filled_qty;
}

int64_t IcebergStrategy::calculateNextSlice() {
    return std::min(display_qty_, remaining_qty_);
}

bool IcebergStrategy::shouldContinue() {
    return remaining_qty_ > 0;
}

} // namespace algo
} // namespace tradex