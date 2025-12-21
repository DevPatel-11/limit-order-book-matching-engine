#include "benchmark.h"
#include "../include/matching_engine.h"
#include <random>
#include <iostream>

class SyntheticWorkloadGenerator {
public:
    SyntheticWorkloadGenerator(uint64_t num_orders, int64_t price_range = 10000000)
        : num_orders(num_orders), price_range(price_range),
          gen(std::random_device{}()), 
          price_dist(price_range, price_range * 2),
          qty_dist(1, 1000),
          side_dist(0, 1) {}
    
    struct Order {
        OrderSide side;
        int64_t price;
        uint64_t qty;
    };
    
    Order next_order() {
        Order o;
        o.side = (side_dist(gen) == 0) ? OrderSide::BUY : OrderSide::SELL;
        o.price = price_dist(gen);
        o.qty = qty_dist(gen);
        return o;
    }
    
    uint64_t get_order_count() const { return num_orders; }
    
private:
    uint64_t num_orders;
    int64_t price_range;
    std::mt19937_64 gen;
    std::uniform_int_distribution<int64_t> price_dist;
    std::uniform_int_distribution<uint64_t> qty_dist;
    std::uniform_int_distribution<int> side_dist;
};

void benchmark_lob(uint64_t num_orders) {
    SyntheticWorkloadGenerator generator(num_orders);
    MatchingEngine engine;
    PerformanceStats stats;
    
    std::cout << "\n====== LOB Benchmark ======" << std::endl;
    std::cout << "Number of orders: " << num_orders << std::endl;
    
    BenchmarkTimer global_timer;
    
    for (uint64_t i = 0; i < num_orders; ++i) {
        auto order = generator.next_order();
        
        BenchmarkTimer timer;
        if (rand() % 100 < 10) {  // 10% market orders
            engine.submitMarketOrder(order.side, order.qty);
        } else {
            engine.submitLimitOrder(order.side, order.price, order.qty);
        }
        timer.stop();
        
        stats.add_latency(timer.elapsed_ns());
    }
    
    global_timer.stop();
    
    stats.compute();
    stats.print_summary("Order Submission Latency");
    
    double throughput = (num_orders * 1e9) / global_timer.elapsed_ns();
    std::cout << "\nThroughput: " << std::fixed << std::setprecision(0)
              << throughput << " orders/sec" << std::endl;
}

int main(int argc, char** argv) {
    uint64_t num_orders = (argc > 1) ? std::stoull(argv[1]) : 100000;
    
    benchmark_lob(num_orders);
    
    return 0;
}
