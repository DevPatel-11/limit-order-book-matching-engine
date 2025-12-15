#include "matching_engine.h"
#include <iostream>

void runDemo() {
    MatchingEngine engine;
    
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "  LIMIT ORDER BOOK & MATCHING ENGINE DEMO" << std::endl;
    std::cout << std::string(50, '=') << "\n" << std::endl;
    
    // Phase 1: Build the order book
    std::cout << "\n>>> Phase 1: Building Order Book\n" << std::endl;
    
    engine.submitLimitOrder(OrderSide::BUY, 100.00, 50);
    engine.submitLimitOrder(OrderSide::BUY, 99.50, 100);
    engine.submitLimitOrder(OrderSide::BUY, 99.00, 75);
    
    engine.submitLimitOrder(OrderSide::SELL, 101.00, 60);
    engine.submitLimitOrder(OrderSide::SELL, 101.50, 80);
    engine.submitLimitOrder(OrderSide::SELL, 102.00, 100);
    
    engine.printBook();
    engine.printStats();
    
    // Phase 2: Match with limit orders
    std::cout << "\n>>> Phase 2: Matching Limit Orders\n" << std::endl;
    
    engine.submitLimitOrder(OrderSide::BUY, 101.25, 90);
    engine.printBook();
    
    engine.submitLimitOrder(OrderSide::SELL, 99.75, 120);
    engine.printBook();
    
    // Phase 3: Market orders
    std::cout << "\n>>> Phase 3: Market Orders\n" << std::endl;
    
    engine.submitMarketOrder(OrderSide::BUY, 50);
    engine.printBook();
    
    engine.submitMarketOrder(OrderSide::SELL, 30);
    engine.printBook();
    
    // Final state
    engine.printTrades();
    engine.printStats();
    
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "  Demo Complete" << std::endl;
    std::cout << std::string(50, '=') << "\n" << std::endl;
}

int main() {
    runDemo();
    return 0;
}
