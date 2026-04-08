#pragma once

#include "tradex/trade/trade.h"
#include "tradex/framework/status.h"
#include <memory>
#include <string>

namespace tradex {
namespace trade {

// Trade repository interface
class ITradeRepository {
public:
    virtual ~ITradeRepository() = default;
    virtual bool save(const Trade& trade) = 0;
    virtual std::vector<Trade> findByAccount(int64_t account_id) = 0;
    virtual std::vector<Trade> findByOrder(int64_t order_id) = 0;
};

// Position repository interface
class IPositionRepository {
public:
    virtual ~IPositionRepository() = default;
    virtual bool save(const Position& position) = 0;
    virtual bool update(const Position& position) = 0;
    virtual std::unique_ptr<Position> find(int64_t account_id, const std::string& symbol) = 0;
    virtual std::vector<Position> findAll(int64_t account_id) = 0;
};

// Asset repository interface
class IAssetRepository {
public:
    virtual ~IAssetRepository() = default;
    virtual bool save(const Asset& asset) = 0;
    virtual bool update(const Asset& asset) = 0;
    virtual std::unique_ptr<Asset> find(int64_t account_id) = 0;
};

// In-memory implementations
class InMemoryTradeRepository : public ITradeRepository {
public:
    bool save(const Trade& trade) override;
    std::vector<Trade> findByAccount(int64_t account_id) override;
    std::vector<Trade> findByOrder(int64_t order_id) override;

private:
    std::vector<Trade> trades_;
    mutable std::mutex mutex_;
};

class InMemoryPositionRepository : public IPositionRepository {
public:
    bool save(const Position& position) override;
    bool update(const Position& position) override;
    std::unique_ptr<Position> find(int64_t account_id, const std::string& symbol) override;
    std::vector<Position> findAll(int64_t account_id) override;

private:
    std::unordered_map<std::pair<int64_t, std::string>, Position, pair_hash> positions_;
    mutable std::mutex mutex_;
};

class InMemoryAssetRepository : public IAssetRepository {
public:
    bool save(const Asset& asset) override;
    bool update(const Asset& asset) override;
    std::unique_ptr<Asset> find(int64_t account_id) override;

private:
    std::unordered_map<int64_t, Asset> assets_;
    mutable std::mutex mutex_;
};

} // namespace trade
} // namespace tradex