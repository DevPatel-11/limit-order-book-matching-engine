#ifndef MATCHING_ENGINE_H
#define MATCHING_ENGINE_H

#include "orderbook.h"
#include <atomic>
#include <chrono>

class MatchingEngine {
private:
    OrderBook orderbook;
    std::atomic<uint64_t> order_id_counter;
    
    uint64_t getCurrentTimestamp() const;
    uint64_t generateOrderId();
    
public:
    MatchingEngine();
    
    uint64_t submitLimitOrder(OrderSide side, double price, uint64_t quantity);
    void submitMarketOrder(OrderSide side, uint64_t quantity);
    
    // New order management methods
    bool cancelOrder(uint64_t order_id);
    bool modifyOrder(uint64_t order_id, double new_price, uint64_t new_quantity);
    
    void printBook() const;
    void printTrades() const;
    void printStats() const;
};

#endif
