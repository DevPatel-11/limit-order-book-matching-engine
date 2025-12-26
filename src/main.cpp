#include "matching_engine.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

static void runSingleThreaded() {
    std::cout << "\n=== Single-threaded demo ===\n";
    MatchingEngine engine;

    uint64_t id1 = engine.submitLimit(Side::BUY,  990000, 100);
    uint64_t id2 = engine.submitLimit(Side::BUY,  995000,  50);
               engine.submitLimit(Side::BUY,  998000,  75);
    uint64_t id4 = engine.submitLimit(Side::SELL, 1010000, 60);
               engine.submitLimit(Side::SELL, 1015000, 80);

    engine.cancelOrder(id1);
    engine.modifyOrder(id4, 1005000, 40);
    engine.submitLimit(Side::BUY, 1010000, 60);
    engine.cancelOrder(id2);

    engine.printBook();
    engine.printTrades();
    engine.printPoolStats();
}

static void runMultiThreaded() {
    std::cout << "\n=== Multi-threaded demo (4 threads) ===\n";
    MatchingEngine engine(false);   // quiet mode

    auto t_start = std::chrono::steady_clock::now();

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

    auto t_end = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();

    std::cout << "200 orders across 4 threads in " << ms << " ms\n";
    engine.printStats();
    engine.printPoolStats();
}

int main() {
    runSingleThreaded();
    runMultiThreaded();
    return 0;
}
