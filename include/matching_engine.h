#pragma once

#include "orderbook.h"
#include <atomic>
#include <vector>

class MatchingEngine {
public:
    explicit MatchingEngine(bool verbose = true);

    uint64_t submitLimit(Side side, int64_t price, uint64_t qty);
    void     submitMarket(Side side, uint64_t qty);

    void printBook(int levels = 5) const;
    void printTrades()             const;
    void printStats()              const;

    const OrderBook& book() const { return book_; }

private:
    OrderBook             book_;
    std::atomic<uint64_t> next_id_{1};
    bool                  verbose_;

    int64_t  nowUs()  const;
    uint64_t nextId() { return next_id_.fetch_add(1, std::memory_order_relaxed); }
    void logTrades(const std::vector<Trade>& trades) const;
};
