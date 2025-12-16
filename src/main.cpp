#include "matching_engine.h"
#include <iostream>

void runDemo() {
    MatchingEngine engine;
    
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "  LIMIT ORDER BOOK & MATCHING ENGINE DEMO" << std::endl;
    std::cout << std::string(50, '=') << "\n" << std::endl;
    
    // Phase 1: Build the order book
    std::cout << "\n>>> Phase 1: Building Order Book\n" << std::endl;
    
    uint64_t order1 = engine.submitLimitOrder(OrderSide::BUY, 100.00, 50);
    uint64_t order2 = engine.submitLimitOrder(OrderSide::BUY, 99.50, 100);
    uint64_t order3 = engine.submitLimitOrder(OrderSide::BUY, 99.00, 75);
    
    uint64_t order4 = engine.submitLimitOrder(OrderSide::SELL, 101.00, 60);
    uint64_t order5 = engine.submitLimitOrder(OrderSide::SELL, 101.50, 80);
    uint64_t order6 = engine.submitLimitOrder(OrderSide::SELL, 102.00, 100);
    
    engine.printBook();
    engine.printStats();
    
    // Phase 2: Test order cancellation
    std::cout << "\n>>> Phase 2: Testing Order Cancellation\n" << std::endl;
    
    // Cancel a bid order
    engine.cancelOrder(order2);  // Cancel buy order at 99.50
    engine.printBook();
    
    // Try to cancel an already canceled order
    engine.cancelOrder(order2);
    
    // Cancel an ask order
    engine.cancelOrder(order5);  // Cancel sell order at 101.50
    engine.printBook();
    engine.printStats();
    
    // Phase 3: Test order modification
    std::cout << "\n>>> Phase 3: Testing Order Modification\n" << std::endl;
    
    // Modify order 1: change price and quantity
    engine.modifyOrder(order1, 100.50, 75);  // Change from 50@100.00 to 75@100.50
    engine.printBook();
    
    // Modify order 4: make it more competitive
    engine.modifyOrder(order4, 100.75, 50);  // Change from 60@101.00 to 50@100.75
    engine.printBook();
    engine.printStats();
    
    // Phase 4: Matching after modifications
    std::cout << "\n>>> Phase 4: Matching After Modifications\n" << std::endl;
    
    engine.submitLimitOrder(OrderSide::BUY, 101.00, 40);  // Should match with modified order4
    engine.printBook();
    
    // Phase 5: Market orders
    std::cout << "\n>>> Phase 5: Market Orders\n" << std::endl;
    
    engine.submitMarketOrder(OrderSide::BUY, 30);
    engine.printBook();
    
    engine.submitMarketOrder(OrderSide::SELL, 20);
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
