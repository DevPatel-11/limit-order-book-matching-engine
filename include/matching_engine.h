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
    
    void submitLimitOrder(OrderSide side, double price, uint64_t quantity);
    void submitMarketOrder(OrderSide side, uint64_t quantity);
    
    void printBook() const;
    void printTrades() const;
    void printStats() const;
};

#endif
