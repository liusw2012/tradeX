#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <functional>

namespace tradex {
namespace gateway {
// Forward declaration
enum class Exchange;
}

namespace market {

// Market data types
struct Quote {
    std::string symbol;
    double last_price;
    double open_price;
    double high_price;
    double low_price;
    double close_price;
    int64_t volume;
    int64_t turnover;
    int64_t timestamp;
};

struct Tick {
    std::string symbol;
    double price;
    int64_t volume;
    int64_t timestamp;
    bool is_buy;
};

struct OrderBookLevel {
    double price;
    int64_t volume;
};

struct OrderBook {
    std::string symbol;
    std::vector<OrderBookLevel> bids;   // Buy orders (price desc)
    std::vector<OrderBookLevel> asks;   // Sell orders (price asc)
    int64_t timestamp;
};

// Market data service
class MarketDataService {
public:
    MarketDataService();
    ~MarketDataService() = default;

    bool initialize();

    // Subscribe/Unsubscribe
    void subscribe(const std::string& symbol);
    void unsubscribe(const std::string& symbol);
    bool isSubscribed(const std::string& symbol) const;

    // Get market data
    std::shared_ptr<Quote> getQuote(const std::string& symbol);
    std::shared_ptr<OrderBook> getOrderBook(const std::string& symbol);

    // Historical data
    std::vector<Quote> getHistory(const std::string& symbol, int64_t start_time, int64_t end_time);

    // Price limit
    double getPriceLimitUp(const std::string& symbol);
    double getPriceLimitDown(const std::string& symbol);

    // Update callbacks
    using QuoteCallback = std::function<void(const Quote&)>;
    using TickCallback = std::function<void(const Tick&)>;

    void setQuoteCallback(QuoteCallback cb) { quote_cb_ = cb; }
    void setTickCallback(TickCallback cb) { tick_cb_ = cb; }

private:
    void updateQuote(const Quote& quote);
    void processTick(const Tick& tick);

    mutable std::mutex mutex_;

    std::unordered_set<std::string> subscriptions_;
    std::unordered_map<std::string, std::shared_ptr<Quote>> quotes_;
    std::unordered_map<std::string, std::shared_ptr<OrderBook>> order_books_;

    QuoteCallback quote_cb_;
    TickCallback tick_cb_;
};

// Stock status
enum class StockStatus {
    Normal = 0,      // Normal trading
    Suspended = 1,   // Trading suspended
    ST = 2,          // Special treatment
    PT = 3,          // P temporary
    Delisted = 4     // Delisted
};

// Stock info
struct StockInfo {
    std::string symbol;
    std::string name;
    gateway::Exchange exchange;
    StockStatus status;
    double limit_up;   // Daily limit up price
    double limit_down; // Daily limit down price
    int64_t lot_size; // Lot size (100 for A-shares)
    double tick_size;  // Minimum price movement
};

} // namespace market
} // namespace tradex