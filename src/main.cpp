#include "matching_engine.h"
#include <iostream>
#include <thread>
#include <vector>

static void runIcebergDemo() {
    std::cout << "\n=== Iceberg order demo ===\n";
    MatchingEngine engine;

    engine.submitLimit(Side::SELL, 1000000, 30);

    std::cout << "\nIceberg buy: 500 total, 100 visible @ $100.00\n";
    engine.submitIceberg(Side::BUY, 1000000, 500, 100);
    engine.printBook();

    engine.submitLimit(Side::SELL, 1000000, 120);
    engine.submitLimit(Side::SELL, 1000000, 300);
    engine.printTrades();
}

static void runStopLossDemo() {
    std::cout << "\n=== Stop-loss order demo ===\n";
    MatchingEngine engine;

    // Bids at two levels
    engine.submitLimit(Side::BUY, 998000, 100);   // $99.80
    engine.submitLimit(Side::BUY, 995000, 200);   // $99.50

    // Stop-loss sell: triggers when last trade <= $99.50; limit at $99.40
    engine.submitStopLoss(Side::SELL, 995000, 994000, 100);
    std::cout << "Pending stop-loss placed. Book:\n";
    engine.printBook();

    // Exhaust the $99.80 level with a market sell
    std::cout << "\nMarket sell 100 — clears $99.80 bid, last price = $99.80 (no trigger)\n";
    engine.submitMarket(Side::SELL, 100);
    engine.printBook();

    // Now a sell that trades at $99.50 — this hits the trigger
    std::cout << "\nLimit sell 50 @ $99.50 — last price = $99.50, stop-loss triggers!\n";
    engine.submitLimit(Side::SELL, 995000, 50);
    engine.printBook();
    engine.printTrades();
}

static void runMultiThreaded() {
    std::cout << "\n=== Multi-threaded demo ===\n";
    MatchingEngine engine(false);

    std::vector<std::thread> threads;
    for (int t = 0; t < 2; ++t)
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 50; ++i)
                engine.submitLimit(Side::BUY,
                    static_cast<int64_t>(990000 - t * 5000 - i * 1000), 10);
        });
    for (int t = 0; t < 2; ++t)
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 50; ++i)
                engine.submitLimit(Side::SELL,
                    static_cast<int64_t>(1010000 + t * 5000 + i * 1000), 10);
        });

    for (auto& th : threads) th.join();
    engine.printStats();
}

int main() {
    runIcebergDemo();
    runStopLossDemo();
    runMultiThreaded();
    return 0;
}
