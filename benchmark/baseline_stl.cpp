#include "benchmark.h"
#include <map>
#include <queue>
#include <functional>
#include <random>
#include <iostream>

enum class OrderSide { BUY, SELL };

struct OrderLevel {
    std::queue<std::pair<uint64_t, uint64_t>> orders;  // {qty, timestamp}
};

class BaselineSTL {
public:
    void addOrder(OrderSide side, int64_t price, uint64_t qty, uint64_t ts) {
        if (side == OrderSide::BUY) {
            if (bids.find(price) == bids.end()) {
                bids[price] = OrderLevel();
            }
            bids[price].orders.push({qty, ts});
        } else {
            if (asks.find(price) == asks.end()) {
                asks[price] = OrderLevel();
            }
            asks[price].orders.push({qty, ts});
        }
    }
    
private:
    std::map<int64_t, OrderLevel, std::greater<int64_t>> bids;  // descending
    std::map<int64_t, OrderLevel, std::less<int64_t>> asks;     // ascending
};

void benchmark_baseline(uint64_t num_orders) {
    BaselineSTL lob;
    PerformanceStats stats;
    
    std::cout << "\n====== Baseline STL Benchmark ======" << std::endl;
    std::cout << "Number of orders: " << num_orders << std::endl;
    
    std::mt19937_64 gen(std::random_device{}());
    std::uniform_int_distribution<int64_t> price_dist(10000000, 20000000);
    std::uniform_int_distribution<uint64_t> qty_dist(1, 1000);
    std::uniform_int_distribution<int> side_dist(0, 1);
    
    BenchmarkTimer global_timer;
    
    for (uint64_t i = 0; i < num_orders; ++i) {
        int64_t price = price_dist(gen);
        uint64_t qty = qty_dist(gen);
        OrderSide side = (side_dist(gen) == 0) ? OrderSide::BUY : OrderSide::SELL;
        
        BenchmarkTimer timer;
        lob.addOrder(side, price, qty, i);
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
    
    benchmark_baseline(num_orders);
    
    return 0;
}
