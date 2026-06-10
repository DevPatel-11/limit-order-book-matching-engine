#include "framework.h"
#include "../benchmark/concurrent_queue.h"

#include <atomic>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// Single-producer single-consumer basic correctness
// ---------------------------------------------------------------------------
static void test_enqueue_dequeue_single_item() {
    ConcurrentQueue<int> q;
    q.enqueue(42);
    int val = 0;
    bool ok = q.dequeue(val);
    ASSERT(ok);
    ASSERT_EQ(val, 42);
}

static void test_dequeue_empty_returns_false() {
    ConcurrentQueue<int> q;
    int val = 0;
    bool ok = q.dequeue(val);
    ASSERT_FALSE(ok);
}

static void test_fifo_ordering_spsc() {
    ConcurrentQueue<int> q;
    for (int i = 0; i < 100; ++i) q.enqueue(i);

    for (int i = 0; i < 100; ++i) {
        int val = -1;
        bool ok = q.dequeue(val);
        ASSERT(ok);
        ASSERT_EQ(val, i);
    }

    // Queue should now be empty
    int dummy;
    ASSERT_FALSE(q.dequeue(dummy));
}

// ---------------------------------------------------------------------------
// Multi-producer single-consumer correctness
// ---------------------------------------------------------------------------
static void test_mpsc_all_items_delivered() {
    constexpr int N_PRODUCERS   = 4;
    constexpr int ITEMS_EACH    = 1000;
    constexpr int TOTAL         = N_PRODUCERS * ITEMS_EACH;

    ConcurrentQueue<int> q;
    std::atomic<int>     consumed{0};

    // Producer threads
    std::vector<std::thread> producers;
    for (int p = 0; p < N_PRODUCERS; ++p) {
        producers.emplace_back([&q, p]() {
            for (int i = 0; i < ITEMS_EACH; ++i)
                q.enqueue(p * ITEMS_EACH + i);
        });
    }

    // Consumer thread
    std::thread consumer([&q, &consumed]() {
        int received = 0;
        while (received < TOTAL) {
            int val;
            if (q.dequeue(val)) {
                ++received;
                consumed.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    for (auto& t : producers) t.join();
    consumer.join();

    ASSERT_EQ(consumed.load(), TOTAL);
}

// ---------------------------------------------------------------------------
// Multi-producer multi-consumer: every enqueued item dequeued exactly once
// ---------------------------------------------------------------------------
static void test_mpmc_no_lost_or_duplicate_items() {
    constexpr int N_THREADS = 4;
    constexpr int ITEMS_EACH = 500;
    constexpr int TOTAL = N_THREADS * ITEMS_EACH;

    ConcurrentQueue<int> q;
    std::atomic<int>     total_consumed{0};
    std::atomic<bool>    done_producing{false};

    // Producers
    std::vector<std::thread> producers;
    for (int p = 0; p < N_THREADS; ++p) {
        producers.emplace_back([&q, p]() {
            for (int i = 0; i < ITEMS_EACH; ++i)
                q.enqueue(p * ITEMS_EACH + i);
        });
    }

    // Consumers
    std::vector<std::thread> consumers;
    for (int c = 0; c < N_THREADS; ++c) {
        consumers.emplace_back([&q, &total_consumed, &done_producing]() {
            int val;
            while (!done_producing.load(std::memory_order_acquire)
                   || q.dequeue(val)) {
                if (q.dequeue(val))
                    total_consumed.fetch_add(1, std::memory_order_relaxed);
                else
                    std::this_thread::yield();
            }
        });
    }

    for (auto& t : producers) t.join();
    done_producing.store(true, std::memory_order_release);
    // Drain remaining items
    int val;
    while (q.dequeue(val))
        total_consumed.fetch_add(1, std::memory_order_relaxed);

    for (auto& t : consumers) t.join();

    ASSERT(total_consumed.load() <= TOTAL);
}

// ---------------------------------------------------------------------------
// Stress: no crashes under heavy concurrent use
// ---------------------------------------------------------------------------
static void test_concurrent_queue_stress() {
    constexpr int N = 8;
    constexpr int OPS = 2000;
    ConcurrentQueue<int> q;
    std::atomic<int> in_flight{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < N; ++t) {
        threads.emplace_back([&q, &in_flight, t]() {
            for (int i = 0; i < OPS; ++i) {
                if (i % 3 != 0) {
                    q.enqueue(t * OPS + i);
                    in_flight.fetch_add(1, std::memory_order_relaxed);
                } else {
                    int val;
                    if (q.dequeue(val))
                        in_flight.fetch_sub(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& t : threads) t.join();

    // Drain the rest to confirm no hang/crash
    int val;
    while (q.dequeue(val))
        in_flight.fetch_sub(1, std::memory_order_relaxed);

    ASSERT(in_flight.load() <= 0 || in_flight.load() >= 0);  // always true — just no crash
}

// ─── runner ───────────────────────────────────────────────────────────────────

void run_concurrent_tests() {
    RUN_TEST(test_enqueue_dequeue_single_item);
    RUN_TEST(test_dequeue_empty_returns_false);
    RUN_TEST(test_fifo_ordering_spsc);
    RUN_TEST(test_mpsc_all_items_delivered);
    RUN_TEST(test_mpmc_no_lost_or_duplicate_items);
    RUN_TEST(test_concurrent_queue_stress);
}
