#pragma once

#include "concurrent_queue.h"
#include "order_event.h"
#include "matching_engine.h"

#include <atomic>
#include <thread>

// ---------------------------------------------------------------------------
// ConcurrentMatchingEngine
//
// Wraps the single-threaded MatchingEngine behind a lock-free event queue.
// Multiple producer threads enqueue OrderEvents; a single dedicated consumer
// thread dequeues and dispatches them to the underlying engine.
//
// This decouples order submission from order processing: producers never
// block on matching logic. The queue absorbs burst traffic, and the consumer
// drains it at engine speed.
//
// Lifecycle:
//   1. Construct.
//   2. Producer threads call submitLimit / submitMarket / cancelOrder.
//   3. Call stop() — drains the queue, joins the consumer thread.
//   4. Query stats or printStats().
// ---------------------------------------------------------------------------
class ConcurrentMatchingEngine {
public:
    explicit ConcurrentMatchingEngine(bool verbose = false);
    ~ConcurrentMatchingEngine();

    ConcurrentMatchingEngine(const ConcurrentMatchingEngine&)            = delete;
    ConcurrentMatchingEngine& operator=(const ConcurrentMatchingEngine&) = delete;

    // Producer API — thread-safe, non-blocking (only enqueue).
    void submitLimit(Side side, int64_t price, uint64_t qty);
    void submitMarket(Side side, uint64_t qty);
    void cancelOrder(uint64_t engine_order_id);

    // Enqueue a sentinel and wait for the consumer to drain and exit.
    void stop();

    // Counters — readable from any thread after stop().
    uint64_t eventsProcessed() const { return events_processed_.load(std::memory_order_relaxed); }

    void printStats() const;

private:
    static constexpr uint8_t SENTINEL = 255;   // EventKind value used as stop signal

    MatchingEngine              engine_;
    ConcurrentQueue<OrderEvent> queue_;
    std::thread                 consumer_;
    std::atomic<bool>           running_{true};
    std::atomic<uint64_t>       events_processed_{0};

    void consumerLoop();
};
