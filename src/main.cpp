#include "matching_engine.h"
#include <iostream>
#include <thread>
#include <vector>

static void runIcebergDemo() {
    std::cout << "\n=== Iceberg order demo ===\n";
    MatchingEngine engine;

    // Build the ask side
    engine.submitLimit(Side::SELL, 1000000, 30);  // $100.00 x 30

    // Iceberg buy: 500 total, only 100 visible at a time
    std::cout << "\nSubmitting iceberg buy: 500 total, 100 visible @ $100.00\n";
    uint64_t ice_id = engine.submitIceberg(Side::BUY, 1000000, 500, 100);
    engine.printBook();

    // Several sells match against the iceberg — trigger replenishment
    std::cout << "\nSell 120 @ $100.00 — exhausts visible lot, triggers replenish\n";
    engine.submitLimit(Side::SELL, 1000000, 120);
    engine.printBook();

    std::cout << "\nSell 300 @ $100.00 — drains 3 more lots\n";
    engine.submitLimit(Side::SELL, 1000000, 300);
    engine.printBook();
    engine.printTrades();

    (void)ice_id;
}

static void runMultiThreaded() {
    std::cout << "\n=== Multi-threaded demo (4 threads) ===\n";
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
    engine.printPoolStats();
}

int main() {
    runIcebergDemo();
    runMultiThreaded();
    return 0;
}
