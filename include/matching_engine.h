#pragma once

#include "orderbook.h"
#include "memory_pool.h"
#include <atomic>
#include <vector>

class MatchingEngine {
public:
    explicit MatchingEngine(bool verbose = true);

    uint64_t submitLimit(Side side, int64_t price, uint64_t qty);
    void     submitMarket(Side side, uint64_t qty);
    uint64_t submitIceberg(Side side, int64_t price,
                           uint64_t total_qty, uint64_t display_qty);
    uint64_t submitStopLoss(Side side, int64_t trigger_price,
                            int64_t limit_price, uint64_t qty);

    bool cancelOrder(uint64_t order_id);
    bool modifyOrder(uint64_t order_id, int64_t new_price, uint64_t new_qty);

    void printBook(int levels = 5) const;
    void printTrades()             const;
    void printStats()              const;
    void printPoolStats()          const;

    const OrderBook& book() const { return book_; }

private:
    OrderBook             book_;
    std::atomic<uint64_t> next_id_{1};
    MemoryPool<Order>     pool_;
    bool                  verbose_;

    int64_t  nowUs()  const;
    uint64_t nextId() { return next_id_.fetch_add(1, std::memory_order_relaxed); }
    void logTrades(const std::vector<Trade>& trades) const;
};
