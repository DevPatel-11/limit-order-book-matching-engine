#pragma once

#include "order.h"
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

struct Trade {
    uint64_t trade_id;
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    int64_t  price;
    uint64_t quantity;
    int64_t  timestamp_us;
};

using OrderPtr = std::shared_ptr<Order>;

class OrderBook {
public:
    OrderBook() = default;
    OrderBook(const OrderBook&)            = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    // Returns trades generated. Stop-loss orders are held until triggered.
    std::vector<Trade> match(OrderPtr order);

    bool cancelOrder(uint64_t order_id);
    bool modifyOrder(uint64_t order_id, int64_t new_price,
                     uint64_t new_qty, int64_t new_timestamp_us);

    int64_t bestBid()     const;
    int64_t bestAsk()     const;
    int64_t spread()      const;
    size_t  bidLevels()   const;
    size_t  askLevels()   const;
    size_t  activeOrders() const;

    const std::vector<Trade>& tradeHistory() const { return trades_; }

    void printBook(int levels = 5)  const;
    void printDepth(int levels = 5) const;
    void printTrades()              const;

private:
    using Level = std::deque<OrderPtr>;

    mutable std::shared_mutex                       mutex_;
    std::map<int64_t, Level, std::greater<int64_t>> bids_;
    std::map<int64_t, Level>                        asks_;
    std::unordered_map<uint64_t, OrderPtr>          active_;
    std::vector<OrderPtr>                           pending_stops_;
    std::vector<OrderPtr>                           triggered_stops_;   // staged for matching
    std::vector<Trade>                              trades_;
    uint64_t                                        next_trade_id_{1};

    void               addToBook(OrderPtr order);
    bool               cancelLocked(uint64_t order_id);
    void               checkStopTriggers(int64_t last_traded_price);
    Trade              makeTrade(uint64_t buy_id, uint64_t sell_id,
                                 int64_t price, uint64_t qty, int64_t ts);
    std::vector<Trade> matchBuy(OrderPtr order);
    std::vector<Trade> matchSell(OrderPtr order);
};
