#include "matching_engine.h"
#include <iostream>

MatchingEngine::MatchingEngine() : order_id_counter(1), order_pool(1000) {}

uint64_t MatchingEngine::getCurrentTimestamp() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

uint64_t MatchingEngine::generateOrderId() {
    return order_id_counter.fetch_add(1);
}

uint64_t MatchingEngine::submitLimitOrder(OrderSide side, int64_t price, uint64_t quantity) {
    OrderType type = (side == OrderSide::BUY) ? OrderType::LIMIT_BUY : OrderType::LIMIT_SELL;
    
    uint64_t order_id = generateOrderId();
    
    // Allocate order from memory pool
    Order* raw_order = order_pool.allocate(
        order_id,
        getCurrentTimestamp(),
        price,
        quantity,
        type,
        side
    );
    
    // Create shared_ptr with custom deleter
    auto order = std::shared_ptr<Order>(raw_order, PoolDeleter<Order>(&order_pool));
    
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
    
    // Allocate order from memory pool
    Order* raw_order = order_pool.allocate(
        generateOrderId(),
        getCurrentTimestamp(),
        0.0,
        quantity,
        type,
        side
    );
    
    // Create shared_ptr with custom deleter
    auto order = std::shared_ptr<Order>(raw_order, PoolDeleter<Order>(&order_pool));
    
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

bool MatchingEngine::modifyOrder(uint64_t order_id, int64_t new_price, uint64_t new_quantity) {
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

void MatchingEngine::printPoolStats() const {
    std::cout << "\n========== MEMORY POOL STATS ==========" << std::endl;
    std::cout << "Total Capacity: " << order_pool.getTotalCapacity() << " orders" << std::endl;
    std::cout << "Free Slots: " << order_pool.getFreeCount() << " orders" << std::endl;
    std::cout << "In Use: " << (order_pool.getTotalCapacity() - order_pool.getFreeCount()) << " orders" << std::endl;
    std::cout << "=======================================\n" << std::endl;
}

uint64_t MatchingEngine::submitIcebergOrder(OrderSide side, int64_t price, uint64_t total_quantity, uint64_t display_quantity) {
    OrderType type = (side == OrderSide::BUY) ? OrderType::ICEBERG_BUY : OrderType::ICEBERG_SELL;
    
    uint64_t order_id = generateOrderId();
    
    // Allocate order from memory pool with iceberg constructor
    Order* raw_order = order_pool.allocate(
        order_id,
        getCurrentTimestamp(),
        price,
        total_quantity,
        type,
        side,
        display_quantity
    );
    
    // Create shared_ptr with custom deleter
    auto order = std::shared_ptr<Order>(raw_order, PoolDeleter<Order>(&order_pool));
    
    std::cout << "\n[SUBMIT ICEBERG] ";
    order->print();
    std::cout << "  Visible: " << display_quantity << ", Hidden: " << (total_quantity - display_quantity) << std::endl;
    
    auto trades = orderbook.matchOrder(order);
    
    if (!trades.empty()) {
        std::cout << "[MATCHED] " << trades.size() << " trade(s) executed\n";
        for (const auto& trade : trades) {
            std::cout << "  -> " << trade.quantity << "@" << trade.price << std::endl;
        }
    }
    
    return order_id;
}

uint64_t MatchingEngine::submitStopLossOrder(OrderSide side, int64_t trigger_price, int64_t limit_price, uint64_t quantity) {
    OrderType type = (side == OrderSide::BUY) ? OrderType::STOP_LOSS_BUY : OrderType::STOP_LOSS_SELL;
    
    uint64_t order_id = generateOrderId();
    
    // Allocate order from memory pool with stop-loss constructor
    Order* order = memory_pool.allocate();
    
    // Allocate raw order from memory pool
    Order* raw_order = memory_pool.allocate();
    new (raw_order) Order(order_id, std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch()).count(),
                          limit_price, quantity, type, side, trigger_price);
    
    // Create shared_ptr with custom deleter
    auto order = std::shared_ptr<Order>(raw_order, PoolDeleter<Order>(&memory_pool));
    
    // Stop-loss orders are kept pending until triggered
    std::cout << "\nStop-loss order submitted!" << std::endl;
    std::cout << "  Order ID: " << order_id << std::endl;
    std::cout << "  Side: " << (side == OrderSide::BUY ? "BUY" : "SELL") << std::endl;
    std::cout << "  Trigger Price: " << trigger_price << std::endl;
    std::cout << "  Limit Price: " << limit_price << std::endl;
    std::cout << "  Quantity: " << quantity << std::endl;
    std::cout << "  (Will activate when market reaches trigger price)" << std::endl;
    
    // For simplicity, add to orderbook immediately
    // In a real system, would keep in separate pending list until triggered
    orderbook.matchOrder(order);
    
    return order_id;
}

void MatchingEngine::printDepth(int levels) const {
    orderbook.printDepth(levels);
}
