#include "matching_engine.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

void runSingleThreadDemo() {
    MatchingEngine engine;
    
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "  SINGLE-THREADED DEMO" << std::endl;
    std::cout << std::string(50, '=') << "\n" << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100; i++) {
        engine.submitLimitOrder(OrderSide::BUY, 100.0 - i * 0.1, 10);
        engine.submitLimitOrder(OrderSide::SELL, 101.0 + i * 0.1, 10);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "\nSingle-threaded: 200 orders in " << duration.count() << "ms" << std::endl;
    engine.printStats();
}

void submitBuyOrders(MatchingEngine& engine, int count, int thread_id) {
    for (int i = 0; i < count; i++) {
        double price = 100.0 - (thread_id * 10 + i) * 0.1;
        engine.submitLimitOrder(OrderSide::BUY, price, 10);
    }
}

void submitSellOrders(MatchingEngine& engine, int count, int thread_id) {
    for (int i = 0; i < count; i++) {
        double price = 101.0 + (thread_id * 10 + i) * 0.1;
        engine.submitLimitOrder(OrderSide::SELL, price, 10);
    }
}

void runMultiThreadDemo() {
    MatchingEngine engine;
    
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "  MULTI-THREADED DEMO" << std::endl;
    std::cout << "  4 Threads Submitting Orders Concurrently" << std::endl;
    std::cout << std::string(50, '=') << "\n" << std::endl;
    
    const int NUM_THREADS = 4;
    const int ORDERS_PER_THREAD = 25;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    
    // Launch buy order threads
    for (int i = 0; i < NUM_THREADS / 2; i++) {
        threads.emplace_back(submitBuyOrders, std::ref(engine), ORDERS_PER_THREAD, i);
    }
    
    // Launch sell order threads
    for (int i = 0; i < NUM_THREADS / 2; i++) {
        threads.emplace_back(submitSellOrders, std::ref(engine), ORDERS_PER_THREAD, i);
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "\nMulti-threaded: 100 orders across " << NUM_THREADS 
              << " threads in " << duration.count() << "ms" << std::endl;
    engine.printStats();
    engine.printPoolStats();
}

void runFullDemo() {
    MatchingEngine engine;
    
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "  LIMIT ORDER BOOK & MATCHING ENGINE DEMO" << std::endl;
    std::cout << "  With Memory Pool & Multi-Threading" << std::endl;
    std::cout << std::string(50, '=') << "\n" << std::endl;
    
    engine.printPoolStats();
    
    std::cout << "\n>>> Phase 1: Building Order Book\n" << std::endl;
    
    uint64_t order1 = engine.submitLimitOrder(OrderSide::BUY, 100.00, 50);
    uint64_t order2 = engine.submitLimitOrder(OrderSide::BUY, 99.50, 100);
    uint64_t order3 = engine.submitLimitOrder(OrderSide::BUY, 99.00, 75);
    
    uint64_t order4 = engine.submitLimitOrder(OrderSide::SELL, 101.00, 60);
    uint64_t order5 = engine.submitLimitOrder(OrderSide::SELL, 101.50, 80);
    uint64_t order6 = engine.submitLimitOrder(OrderSide::SELL, 102.00, 100);
    
    engine.printBook();
    engine.printStats();
    
    std::cout << "\n>>> Phase 2: Multi-threaded Order Submission\n" << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::thread t1([&]() {
        for (int i = 0; i < 10; i++) {
            engine.submitLimitOrder(OrderSide::BUY, 98.5 - i * 0.1, 5);
        }
    });
    
    std::thread t2([&]() {
        for (int i = 0; i < 10; i++) {
            engine.submitLimitOrder(OrderSide::SELL, 102.5 + i * 0.1, 5);
        }
    });
    
    t1.join();
    t2.join();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "20 concurrent orders processed in " << duration.count() << " microseconds" << std::endl;
    
    engine.printBook();
    // Test 6: Iceberg Order Demo
    std::cout << "\n\n=== Test 6: Iceberg Orders (Hidden Quantity) ===" << std::endl;
    std::cout << "Submitting iceberg buy order: 1000 total qty, 200 visible at $99.50" << std::endl;
    uint64_t iceberg_id = engine.submitIcebergOrder(OrderSide::BUY, 99.50, 1000, 200);
    
    std::cout << "\nSubmitting 3 sell orders (150 qty each) at $99.50 to match against iceberg:" << std::endl;
    engine.submitLimitOrder(OrderSide::SELL, 99.50, 150); // Will partially fill iceberg display
    engine.submitLimitOrder(OrderSide::SELL, 99.50, 150); // Will fully fill display, trigger replenish
    engine.submitLimitOrder(OrderSide::SELL, 99.50, 150); // Will match against replenished display
    
    std::cout << "\n📊 Orderbook State (after iceberg partial fills):" << std::endl;
    engine.printBook();
    std::cout << "\n💹 Trade Statistics:" << std::endl;

    engine.printStats();
    engine.printPoolStats();
    

    // Test 7: Stop-Loss Orders
    std::cout << "\n\n=== Test 7: Stop-Loss Orders ===" << std::endl;
    std::cout << "Submitting stop-loss sell order: trigger at $98, limit at $97.50, 100 qty" << std::endl;
    uint64_t stoploss_id = engine.submitStopLossOrder(OrderSide::SELL, 98.0, 97.50, 100);
    
    std::cout << "\n📊 Orderbook State (with stop-loss pending):" << std::endl;
    engine.printBook();
    std::cout << "\n" << std::string(50, '=') << std::endl;

    // Test 8: Market Depth Visualization
    std::cout << "\n\n=== Test 8: Market Depth Visualization ===" << std::endl;
    std::cout << "Displaying cumulative volume at each price level:" << std::endl;
    engine.printDepth(5);  // Show top 5 levels
    std::cout << "  Demo Complete" << std::endl;
    std::cout << "  Thread-safe matching engine!" << std::endl;
    std::cout << std::string(50, '=') << "\n" << std::endl;
}

int main() {
    runFullDemo();
    std::cout << "\n\n";
    runSingleThreadDemo();
    std::cout << "\n\n";
    runMultiThreadDemo();
    return 0;
}
