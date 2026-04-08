#include "tradex/market/market.h"
#include "tradex/framework/log.h"
#include <random>

namespace tradex {
namespace market {

MarketDataService::MarketDataService() {}

bool MarketDataService::initialize() {
    LOG_INFO("MarketDataService initialized");
    return true;
}

void MarketDataService::subscribe(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_.insert(symbol);
    LOG_INFO("Subscribed to market data: " + symbol);
}

void MarketDataService::unsubscribe(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_.erase(symbol);
    LOG_INFO("Unsubscribed from market data: " + symbol);
}

bool MarketDataService::isSubscribed(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    return subscriptions_.find(symbol) != subscriptions_.end();
}

std::shared_ptr<Quote> MarketDataService::getQuote(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = quotes_.find(symbol);
    if (it != quotes_.end()) {
        return it->second;
    }

    // Generate dummy quote for testing
    auto quote = std::make_shared<Quote>();
    quote->symbol = symbol;
    quote->last_price = 100.0;
    quote->open_price = 99.5;
    quote->high_price = 101.0;
    quote->low_price = 99.0;
    quote->close_price = 100.0;
    quote->volume = 1000000;
    quote->turnover = 100000000;
    quote->timestamp = std::time(nullptr);

    quotes_[symbol] = quote;
    return quote;
}

std::shared_ptr<OrderBook> MarketDataService::getOrderBook(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = order_books_.find(symbol);
    if (it != order_books_.end()) {
        return it->second;
    }

    // Generate dummy order book
    auto book = std::make_shared<OrderBook>();
    book->symbol = symbol;
    book->timestamp = std::time(nullptr);

    // Generate 5 levels on each side
    double base_price = 100.0;
    for (int i = 0; i < 5; ++i) {
        OrderBookLevel bid;
        bid.price = base_price - i * 0.01;
        bid.volume = 1000 * (i + 1);
        book->bids.push_back(bid);

        OrderBookLevel ask;
        ask.price = base_price + 0.01 + i * 0.01;
        ask.volume = 1000 * (i + 1);
        book->asks.push_back(ask);
    }

    order_books_[symbol] = book;
    return book;
}

std::vector<Quote> MarketDataService::getHistory(const std::string& symbol,
                                                   int64_t start_time,
                                                   int64_t end_time) {
    // Placeholder - would fetch from database
    return {};
}

double MarketDataService::getPriceLimitUp(const std::string& symbol) {
    auto quote = getQuote(const_cast<std::string&>(symbol));  // Lock-free for now
    if (quote && quote->close_price > 0) {
        return quote->close_price * 1.1;  // 10% for A-shares
    }
    return 0.0;
}

double MarketDataService::getPriceLimitDown(const std::string& symbol) {
    auto quote = getQuote(const_cast<std::string&>(symbol));
    if (quote && quote->close_price > 0) {
        return quote->close_price * 0.9;  // 10% for A-shares
    }
    return 0.0;
}

void MarketDataService::updateQuote(const Quote& quote) {
    std::lock_guard<std::mutex> lock(mutex_);
    quotes_[quote.symbol] = std::make_shared<Quote>(quote);

    if (quote_cb_) {
        quote_cb_(quote);
    }
}

void MarketDataService::processTick(const Tick& tick) {
    if (tick_cb_) {
        tick_cb_(tick);
    }
}

} // namespace market
} // namespace tradex