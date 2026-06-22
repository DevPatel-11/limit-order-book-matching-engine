// Baseline: naive STL implementation of a matching engine for comparison.
// Uses std::multimap and std::make_shared per order (no memory pool, no FIFO deque).
// Compiled separately to measure the overhead our optimisations avoid.

#include "benchmark.h"
#include <map>
#include <queue>
#include <memory>
#include <random>
#include <iostream>

struct SimpleOrder {
    uint64_t id;
    int64_t  price;
    uint64_t qty;
};

struct SimpleTrade {
    uint64_t buy_id, sell_id;
    int64_t  price;
    uint64_t qty;
};

class BaselineLOB {
public:
    std::vector<SimpleTrade> match(bool is_buy, int64_t price, uint64_t qty) {
        std::vector<SimpleTrade> trades;
        uint64_t id = next_id_++;

        if (is_buy) {
            while (qty > 0 && !asks_.empty()) {
                auto it = asks_.begin();
                if (price < it->first) break;
                auto& q = it->second;
                while (!q.empty() && qty > 0) {
                    auto& resting = q.front();
                    uint64_t tqty = std::min(qty, resting.qty);
                    qty         -= tqty;
                    resting.qty -= tqty;
                    trades.push_back({id, resting.id, it->first, tqty});
                    if (resting.qty == 0) q.pop();
                }
                if (q.empty()) asks_.erase(it);
            }
            if (qty > 0) bids_[price].push({id, price, qty});
        } else {
            while (qty > 0 && !bids_.empty()) {
                auto it = bids_.begin();
                if (price > it->first) break;
                auto& q = it->second;
                while (!q.empty() && qty > 0) {
                    auto& resting = q.front();
                    uint64_t tqty = std::min(qty, resting.qty);
                    qty         -= tqty;
                    resting.qty -= tqty;
                    trades.push_back({resting.id, id, it->first, tqty});
                    if (resting.qty == 0) q.pop();
                }
                if (q.empty()) bids_.erase(it);
            }
            if (qty > 0) asks_[price].push({id, price, qty});
        }
        return trades;
    }

private:
    std::map<int64_t, std::queue<SimpleOrder>, std::greater<int64_t>> bids_;
    std::map<int64_t, std::queue<SimpleOrder>>                        asks_;
    uint64_t next_id_{1};
};

static void run_baseline(uint64_t n) {
    std::mt19937_64 rng{42};
    std::uniform_int_distribution<int64_t>  price_dist(9900000, 10100000);
    std::uniform_int_distribution<uint64_t> qty_dist(1, 500);
    std::uniform_int_distribution<int>      side_dist(0, 1);

    BaselineLOB lob;
    PerformanceStats stats;
    BenchmarkTimer total;

    for (uint64_t i = 0; i < n; ++i) {
        BenchmarkTimer t;
        lob.match(side_dist(rng) == 0,
                  price_dist(rng),
                  qty_dist(rng));
        t.stop();
        stats.add_latency(t.elapsed_ns());
    }

    total.stop();
    stats.compute();
    stats.print_summary("Baseline STL LOB");
    double throughput = n * 1e9 / total.elapsed_ns();
    std::cout << "Throughput: " << static_cast<uint64_t>(throughput) << " orders/sec\n";
}

int main(int argc, char** argv) {
    uint64_t n = (argc > 1) ? std::stoull(argv[1]) : 100000;
    std::cout << "=== Baseline STL benchmark (" << n << " orders) ===\n";
    run_baseline(n);
    return 0;
}
