#include "concurrent_matching_engine.h"
#include "benchmark.h"

#include <iostream>
#include <random>
#include <thread>
#include <vector>
#include <atomic>

// ---------------------------------------------------------------------------
// ConcurrentMatchingEngine implementation
// ---------------------------------------------------------------------------

ConcurrentMatchingEngine::ConcurrentMatchingEngine(bool verbose)
    : engine_(verbose)
{
    consumer_ = std::thread(&ConcurrentMatchingEngine::consumerLoop, this);
}

ConcurrentMatchingEngine::~ConcurrentMatchingEngine() {
    if (running_.load(std::memory_order_relaxed)) stop();
}

void ConcurrentMatchingEngine::submitLimit(Side side, int64_t price, uint64_t qty) {
    OrderEvent ev;
    ev.kind     = EventKind::SUBMIT_LIMIT;
    ev.order_id = 0;   // assigned by engine on dispatch
    ev.side     = (side == Side::BUY) ? 'B' : 'S';
    ev.price    = price;
    ev.quantity = qty;
    queue_.enqueue(ev);
}

void ConcurrentMatchingEngine::submitMarket(Side side, uint64_t qty) {
    OrderEvent ev;
    ev.kind     = EventKind::SUBMIT_MARKET;
    ev.order_id = 0;
    ev.side     = (side == Side::BUY) ? 'B' : 'S';
    ev.price    = 0;
    ev.quantity = qty;
    queue_.enqueue(ev);
}

void ConcurrentMatchingEngine::cancelOrder(uint64_t engine_order_id) {
    OrderEvent ev;
    ev.kind     = EventKind::CANCEL;
    ev.order_id = engine_order_id;
    ev.side     = 'X';
    ev.price    = 0;
    ev.quantity = 0;
    queue_.enqueue(ev);
}

void ConcurrentMatchingEngine::stop() {
    // Push sentinel to signal consumer to exit
    OrderEvent sentinel;
    sentinel.kind = static_cast<EventKind>(SENTINEL);
    queue_.enqueue(sentinel);

    if (consumer_.joinable()) consumer_.join();
    running_.store(false, std::memory_order_relaxed);
}

void ConcurrentMatchingEngine::consumerLoop() {
    OrderEvent ev;
    while (true) {
        if (!queue_.dequeue(ev)) {
            // Spin — yield to avoid burning a full core
            std::this_thread::yield();
            continue;
        }

        if (static_cast<uint8_t>(ev.kind) == SENTINEL) break;

        switch (ev.kind) {
        case EventKind::SUBMIT_LIMIT:
            engine_.submitLimit(
                (ev.side == 'B') ? Side::BUY : Side::SELL,
                ev.price, ev.quantity);
            break;
        case EventKind::SUBMIT_MARKET:
            engine_.submitMarket(
                (ev.side == 'B') ? Side::BUY : Side::SELL,
                ev.quantity);
            break;
        case EventKind::CANCEL:
            engine_.cancelOrder(ev.order_id);
            break;
        default:
            break;
        }

        events_processed_.fetch_add(1, std::memory_order_relaxed);
    }
}

void ConcurrentMatchingEngine::printStats() const {
    std::cout << "\n=== Concurrent Engine Stats ===\n"
              << "  Events processed : " << eventsProcessed() << "\n";
    engine_.printStats();
}

// ---------------------------------------------------------------------------
// Multi-producer benchmark
//
// N producer threads each enqueue orders_per_producer events; one consumer
// thread dispatches them. We measure end-to-end wall-clock throughput.
// ---------------------------------------------------------------------------
void runConcurrentBenchmark(int num_producers, uint64_t orders_per_producer) {
    std::cout << "\n=== Concurrent Benchmark ("
              << num_producers << " producers x "
              << orders_per_producer << " orders) ===\n";

    ConcurrentMatchingEngine cme(false);

    std::vector<std::thread> producers;
    producers.reserve(static_cast<size_t>(num_producers));

    BenchmarkTimer total;

    for (int p = 0; p < num_producers; ++p) {
        producers.emplace_back([&cme, orders_per_producer, p]() {
            std::mt19937_64 rng{static_cast<uint64_t>(p * 0x9e3779b97f4a7c15ULL)};
            std::normal_distribution<double> price_dist(1000000.0, 10000.0);
            std::uniform_int_distribution<uint64_t> qty_dist(1, 200);
            std::uniform_int_distribution<int>      side_dist(0, 1);
            std::uniform_int_distribution<int>      kind_dist(0, 4);

            for (uint64_t i = 0; i < orders_per_producer; ++i) {
                Side side = side_dist(rng) ? Side::BUY : Side::SELL;
                int64_t price = static_cast<int64_t>(price_dist(rng));
                price = (price / 100) * 100;
                if (price <= 0) price = 1000;

                if (kind_dist(rng) == 0) {
                    cme.submitMarket(side, qty_dist(rng));
                } else {
                    cme.submitLimit(side, price, qty_dist(rng));
                }
            }
        });
    }

    for (auto& t : producers) t.join();
    cme.stop();

    total.stop();

    uint64_t total_events = static_cast<uint64_t>(num_producers) * orders_per_producer;
    double throughput = static_cast<double>(total_events) * 1e9 / total.elapsed_ns();

    std::cout << "Total events  : " << total_events << "\n"
              << "Throughput    : " << static_cast<uint64_t>(throughput) << " events/sec\n"
              << "Wall time     : " << total.elapsed_ms() << " ms\n";
    cme.printStats();
}
