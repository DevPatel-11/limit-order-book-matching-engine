#include "matching_engine.h"
#include <iostream>

int main() {
    std::cout << "=== Limit Order Book Matching Engine — Feature 1 Demo ===\n\n";

    MatchingEngine engine;

    // --- Passive limit orders (no crosses, build the book) ---
    std::cout << "\n--- Passive limit buys ---\n";
    engine.submitLimit(Side::BUY, 1000000, 100);   // $100.00 x 100
    engine.submitLimit(Side::BUY,  995000, 200);   // $ 99.50 x 200
    engine.submitLimit(Side::BUY,  990000, 150);   // $ 99.00 x 150

    std::cout << "\n--- Passive limit sells ---\n";
    engine.submitLimit(Side::SELL, 1010000, 120);  // $101.00 x 120
    engine.submitLimit(Side::SELL, 1015000,  80);  // $101.50 x  80

    std::cout << "\n--- Book after passive orders ---\n";
    engine.printBook(5);

    // --- Aggressive limit buy: crosses the spread, sweeps ask levels ---
    std::cout << "\n--- Aggressive limit buy @ $101.50 (sweeps $101.00 and $101.50) ---\n";
    engine.submitLimit(Side::BUY, 1015000, 150);   // will consume 120 @101.00 + 30 @101.50

    std::cout << "\n--- Book after aggressive limit ---\n";
    engine.printBook(5);

    // --- Market sell: hits best bids without a price ---
    std::cout << "\n--- Market sell qty=250 ---\n";
    engine.submitMarket(Side::SELL, 250);          // $100.00 x 100 then $99.50 x 150

    // --- Final state ---
    std::cout << "\n--- Final book state ---\n";
    engine.printBook(5);
    engine.printTrades();
    engine.printStats();

    return 0;
}
