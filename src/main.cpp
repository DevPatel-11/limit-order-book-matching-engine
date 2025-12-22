#include "matching_engine.h"
#include <iostream>

int main() {
    MatchingEngine engine;

    std::cout << "\n=== Phase 1: Build order book ===\n";
    uint64_t id1 = engine.submitLimit(Side::BUY,  990000, 100);   // $99.00
    uint64_t id2 = engine.submitLimit(Side::BUY,  995000,  50);   // $99.50
               engine.submitLimit(Side::BUY,  998000,  75);   // $99.80
    uint64_t id4 = engine.submitLimit(Side::SELL, 1010000, 60);   // $101.00
               engine.submitLimit(Side::SELL, 1015000, 80);   // $101.50
    engine.printBook();

    std::cout << "\n=== Phase 2: Cancel an order ===\n";
    engine.cancelOrder(id1);   // cancel $99.00 buy
    engine.printBook();

    std::cout << "\n=== Phase 3: Modify an order ===\n";
    engine.modifyOrder(id4, 1005000, 40);  // move $101.00 sell to $100.50, reduce qty
    engine.printBook();

    std::cout << "\n=== Phase 4: Aggressive order (crosses spread after modify) ===\n";
    engine.submitLimit(Side::BUY, 1010000, 60);  // sweeps $100.50 ask
    engine.printBook();
    engine.printTrades();
    engine.printStats();
    engine.printPoolStats();

    std::cout << "\n=== Phase 5: Cancel already-filled order ===\n";
    engine.cancelOrder(id2);   // still active — should cancel
    engine.cancelOrder(999);   // non-existent — should report not found
    engine.printBook();
    return 0;
}
