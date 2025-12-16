#include "matching_engine.h"
#include <iostream>

MatchingEngine::MatchingEngine() : order_id_counter(1) {}

uint64_t MatchingEngine::getCurrentTimestamp() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

uint64_t MatchingEngine::generateOrderId() {
    return order_id_counter.fetch_add(1);
}

uint64_t MatchingEngine::submitLimitOrder(OrderSide side, double price, uint64_t quantity) {
    OrderType type = (side == OrderSide::BUY) ? OrderType::LIMIT_BUY : OrderType::LIMIT_SELL;
    
    uint64_t order_id = generateOrderId();
    auto order = std::make_shared<Order>(
        order_id,
        getCurrentTimestamp(),
        price,
        quantity,
        type,
        side
    );
    
    std::cout << "\n[SUBMIT] ";
    order->print();
    
    auto trades = orderbook.matchOrder(order);
    
    if (!trades.empty()) {
        std::cout << "[MATCHED] " << trades.size() << " trade(s) executed\n";
        for (const auto& trade : trades) {
            std::cout << "  -> " << trade.quantity << "@" << trade.price << std::endl;
        }
    }
    
    return order_id;
}

void MatchingEngine::submitMarketOrder(OrderSide side, uint64_t quantity) {
    OrderType type = (side == OrderSide::BUY) ? OrderType::MARKET_BUY : OrderType::MARKET_SELL;
    
    // Market orders use price 0 as placeholder
    auto order = std::make_shared<Order>(
        generateOrderId(),
        getCurrentTimestamp(),
        0.0,
        quantity,
        type,
        side
    );
    
    std::cout << "\n[SUBMIT MARKET] ";
    order->print();
    
    auto trades = orderbook.matchOrder(order);
    
    if (!trades.empty()) {
        std::cout << "[MATCHED] " << trades.size() << " trade(s) executed\n";
        for (const auto& trade : trades) {
            std::cout << "  -> " << trade.quantity << "@" << trade.price << std::endl;
        }
    }
}

bool MatchingEngine::cancelOrder(uint64_t order_id) {
    std::cout << "\n[CANCEL] Attempting to cancel order #" << order_id << std::endl;
    bool success = orderbook.cancelOrder(order_id);
    
    if (success) {
        std::cout << "[SUCCESS] Order #" << order_id << " has been canceled" << std::endl;
    } else {
        std::cout << "[FAILED] Order #" << order_id << " not found or already filled" << std::endl;
    }
    
    return success;
}

bool MatchingEngine::modifyOrder(uint64_t order_id, double new_price, uint64_t new_quantity) {
    std::cout << "\n[MODIFY] Attempting to modify order #" << order_id 
              << " to " << new_quantity << "@" << new_price << std::endl;
    
    bool success = orderbook.modifyOrder(order_id, new_price, new_quantity);
    
    if (success) {
        std::cout << "[SUCCESS] Order #" << order_id << " has been modified" << std::endl;
    } else {
        std::cout << "[FAILED] Order #" << order_id << " not found" << std::endl;
    }
    
    return success;
}

void MatchingEngine::printBook() const {
    orderbook.printBook();
}

void MatchingEngine::printTrades() const {
    orderbook.printTrades();
}

void MatchingEngine::printStats() const {
    std::cout << "\n========== STATISTICS ==========" << std::endl;
    std::cout << "Best Bid: " << orderbook.getBestBid() << std::endl;
    std::cout << "Best Ask: " << orderbook.getBestAsk() << std::endl;
    std::cout << "Spread: " << orderbook.getSpread() << std::endl;
    std::cout << "Bid Depth: " << orderbook.getBidDepth() << " levels" << std::endl;
    std::cout << "Ask Depth: " << orderbook.getAskDepth() << " levels" << std::endl;
    std::cout << "Active Orders: " << orderbook.getActiveOrderCount() << std::endl;
    std::cout << "================================\n" << std::endl;
}
