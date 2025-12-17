#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include "order.h"
#include <map>
#include <queue>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>

using OrderPtr = std::shared_ptr<Order>;

struct Trade {
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    double price;
    uint64_t quantity;
    uint64_t timestamp;
};

class OrderBook {
private:
    // Bids: price -> queue of orders (descending by price)
    std::map<double, std::queue<OrderPtr>, std::greater<double>> bids;
    
    // Asks: price -> queue of orders (ascending by price)
    std::map<double, std::queue<OrderPtr>, std::less<double>> asks;
    
    // Track all active orders by ID for cancellation/modification
    std::unordered_map<uint64_t, OrderPtr> active_orders;
    
    std::vector<Trade> trade_history;
    
    // Thread safety
    mutable std::shared_mutex book_mutex;
    
public:
    OrderBook() = default;
    
    void addOrder(OrderPtr order);
    std::vector<Trade> matchOrder(OrderPtr order);
    
    // Order management
    bool cancelOrder(uint64_t order_id);
    bool modifyOrder(uint64_t order_id, double new_price, uint64_t new_quantity);
    
    double getBestBid() const;
    double getBestAsk() const;
    double getSpread() const;
    
    void printBook() const;
    void printTrades() const;
    
    size_t getBidDepth() const;
    size_t getAskDepth() const;
    size_t getActiveOrderCount() const;
};

#endif
