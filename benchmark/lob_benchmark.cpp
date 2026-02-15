#include "benchmark.h"
#include "matching_engine.h"
#include "order_event.h"
#include "concurrent_matching_engine.h"

#include <algorithm>
#include <deque>
#include <iostream>
#include <random>
#include <vector>

// ---------------------------------------------------------------------------
// Workload generator
//
// Produces a deterministic stream of order events:
//   ~65 % limit order submissions
//   ~20 % market order submissions
//   ~15 % cancel requests (randomly selected from live orders)
//
// Prices are drawn from a normal distribution centred at $100 with a ±2 %
// band so the book stays crossed often enough to generate realistic trades.
// ---------------------------------------------------------------------------
static std::vector<OrderEvent> buildWorkload(uint64_t n, uint64_t seed = 42) {
    std::mt19937_64 rng{seed};

    // Price distribution: centre $100, σ = $1.00 (units: price * PRICE_SCALE)
    constexpr int64_t MID   = 100 * 10000;          // $100.00
    constexpr int64_t SIGMA = 10000;                 // $1.00
    std::normal_distribution<double> price_dist(
        static_cast<double>(MID), static_cast<double>(SIGMA));

    std::uniform_int_distribution<uint64_t> qty_dist(1, 200);
    std::uniform_int_distribution<int>      side_dist(0, 1);
    std::uniform_int_distribution<int>      kind_dist(0, 99);

    // Running pool of order IDs that can be cancelled
    std::deque<uint64_t> live_ids;
    uint64_t next_id = 1;

    std::vector<OrderEvent> events;
    events.reserve(n);

    for (uint64_t i = 0; i < n; ++i) {
        int roll = kind_dist(rng);

        if (roll < 15 && !live_ids.empty()) {
            // Cancel: pick a random live order
            std::uniform_int_distribution<size_t> idx(0, live_ids.size() - 1);
            size_t pos = idx(rng);
            uint64_t oid = live_ids[pos];
            live_ids.erase(live_ids.begin() + static_cast<ptrdiff_t>(pos));

            OrderEvent ev;
            ev.kind       = EventKind::CANCEL;
            ev.order_id   = oid;
            ev.side       = 'X';
            ev.price      = 0;
            ev.quantity   = 0;
            events.push_back(ev);
        } else if (roll < 35) {
            // Market order
            char side = side_dist(rng) ? 'B' : 'S';
            uint64_t qty = qty_dist(rng);

            OrderEvent ev;
            ev.kind       = EventKind::SUBMIT_MARKET;
            ev.order_id   = next_id++;
            ev.side       = side;
            ev.price      = 0;
            ev.quantity   = qty;
            events.push_back(ev);
        } else {
            // Limit order
            char side = side_dist(rng) ? 'B' : 'S';
            int64_t raw_price = static_cast<int64_t>(price_dist(rng));
            // Round to nearest cent (100 ticks = $0.01)
            raw_price = (raw_price / 100) * 100;
            if (raw_price <= 0) raw_price = 100;

            uint64_t qty = qty_dist(rng);
            uint64_t oid = next_id++;

            OrderEvent ev;
            ev.kind       = EventKind::SUBMIT_LIMIT;
            ev.order_id   = oid;
            ev.side       = side;
            ev.price      = raw_price;
            ev.quantity   = qty;
            events.push_back(ev);

            live_ids.push_back(oid);
            // Keep pool bounded so cancel picks remain fast
            if (live_ids.size() > 2000) live_ids.pop_front();
        }
    }

    return events;
}

// ---------------------------------------------------------------------------
// Single-threaded latency benchmark
// ---------------------------------------------------------------------------
static void runLatencyBenchmark(uint64_t n) {
    std::cout << "\n=== LOB Latency Benchmark (" << n << " events) ===\n";

    auto events = buildWorkload(n);

    // verbose=false suppresses all console output in the hot path
    MatchingEngine engine(false);

    PerformanceStats stats;
    BenchmarkTimer   total;

    // Map from workload order_id → engine-assigned order_id (for cancels)
    std::vector<uint64_t> engine_ids(n + 1, 0);

    for (auto& ev : events) {
        BenchmarkTimer t;

        switch (ev.kind) {
        case EventKind::SUBMIT_LIMIT:
        {
            Side side = (ev.side == 'B') ? Side::BUY : Side::SELL;
            uint64_t eid = engine.submitLimit(side, ev.price, ev.quantity);
            if (ev.order_id < engine_ids.size()) engine_ids[ev.order_id] = eid;
            break;
        }
        case EventKind::SUBMIT_MARKET:
        {
            Side side = (ev.side == 'B') ? Side::BUY : Side::SELL;
            engine.submitMarket(side, ev.quantity);
            break;
        }
        case EventKind::CANCEL:
        {
            uint64_t eid = (ev.order_id < engine_ids.size())
                           ? engine_ids[ev.order_id] : 0;
            if (eid > 0) engine.cancelOrder(eid);
            break;
        }
        }

        t.stop();
        stats.add_latency(t.elapsed_ns());
    }

    total.stop();

    stats.compute();
    stats.print_summary("Order processing latency");

    double throughput = static_cast<double>(n) * 1e9 / total.elapsed_ns();
    std::cout << "Throughput : " << static_cast<uint64_t>(throughput)
              << " orders/sec\n";
    std::cout << "Wall time  : " << total.elapsed_ms() << " ms\n";
}

// ---------------------------------------------------------------------------
// Throughput benchmark — maximise total events per second
// ---------------------------------------------------------------------------
static void runThroughputBenchmark(uint64_t n) {
    std::cout << "\n=== LOB Throughput Benchmark (" << n << " events) ===\n";

    auto events = buildWorkload(n, 137);

    MatchingEngine engine(false);
    std::vector<uint64_t> engine_ids(n + 1, 0);

    BenchmarkTimer total;

    for (auto& ev : events) {
        switch (ev.kind) {
        case EventKind::SUBMIT_LIMIT:
        {
            Side side = (ev.side == 'B') ? Side::BUY : Side::SELL;
            uint64_t eid = engine.submitLimit(side, ev.price, ev.quantity);
            if (ev.order_id < engine_ids.size()) engine_ids[ev.order_id] = eid;
            break;
        }
        case EventKind::SUBMIT_MARKET:
        {
            Side side = (ev.side == 'B') ? Side::BUY : Side::SELL;
            engine.submitMarket(side, ev.quantity);
            break;
        }
        case EventKind::CANCEL:
        {
            uint64_t eid = (ev.order_id < engine_ids.size())
                           ? engine_ids[ev.order_id] : 0;
            if (eid > 0) engine.cancelOrder(eid);
            break;
        }
        }
    }

    total.stop();

    double throughput = static_cast<double>(n) * 1e9 / total.elapsed_ns();
    std::cout << "Throughput : " << static_cast<uint64_t>(throughput)
              << " orders/sec\n";
    std::cout << "Wall time  : " << total.elapsed_ms() << " ms\n";
    engine.printStats();
}

// ---------------------------------------------------------------------------
// Modify benchmark — measures cancel+reinsert overhead
// ---------------------------------------------------------------------------
static void runModifyBenchmark(uint64_t n) {
    std::cout << "\n=== Modify Order Benchmark (" << n << " modifies) ===\n";

    MatchingEngine engine(false);

    // Seed the book with resting orders on both sides
    std::vector<uint64_t> bid_ids, ask_ids;
    for (int i = 0; i < 200; ++i) {
        bid_ids.push_back(engine.submitLimit(Side::BUY,
            static_cast<int64_t>(990000 - i * 1000), 100));
        ask_ids.push_back(engine.submitLimit(Side::SELL,
            static_cast<int64_t>(1010000 + i * 1000), 100));
    }

    std::mt19937_64 rng{99};
    std::uniform_int_distribution<size_t> bid_pick(0, bid_ids.size() - 1);
    std::uniform_int_distribution<size_t> ask_pick(0, ask_ids.size() - 1);

    PerformanceStats stats;
    BenchmarkTimer   total;

    for (uint64_t i = 0; i < n; ++i) {
        BenchmarkTimer t;
        if (i % 2 == 0)
            engine.modifyOrder(bid_ids[bid_pick(rng)], 985000LL, 50);
        else
            engine.modifyOrder(ask_ids[ask_pick(rng)], 1015000LL, 50);
        t.stop();
        stats.add_latency(t.elapsed_ns());
    }

    total.stop();
    stats.compute();
    stats.print_summary("Modify order latency");
}

// Declared in concurrent_matching_engine.cpp
void runConcurrentBenchmark(int num_producers, uint64_t orders_per_producer);

int main(int argc, char** argv) {
    uint64_t n = (argc > 1) ? std::stoull(argv[1]) : 200000;

    runLatencyBenchmark(n);
    runThroughputBenchmark(n * 5);
    runModifyBenchmark(50000);
    runConcurrentBenchmark(4, n / 4);

    return 0;
}
