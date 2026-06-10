#include "framework.h"
#include "memory_pool.h"
#include "order.h"

#include <memory>
#include <set>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// Basic allocate / deallocate
// ---------------------------------------------------------------------------
static void test_allocate_returns_non_null() {
    MemoryPool<Order> pool(16);
    Order* p = pool.allocate(1ULL, 0LL, Side::BUY, OrderKind::LIMIT, 1000000LL, 100ULL);
    ASSERT(p != nullptr);
    pool.deallocate(p);
}

static void test_allocated_object_is_valid() {
    MemoryPool<Order> pool(16);
    Order* p = pool.allocate(42ULL, 1000LL, Side::SELL, OrderKind::LIMIT, 2000000LL, 50ULL);
    ASSERT_EQ(p->id(),       42ULL);
    ASSERT(p->side() == Side::SELL);
    ASSERT_EQ(p->price(),    2000000LL);
    ASSERT_EQ(p->quantity(), 50ULL);
    pool.deallocate(p);
}

static void test_capacity_reported_correctly() {
    MemoryPool<Order> pool(10);
    ASSERT_EQ(pool.totalCapacity(), 10ULL);
    ASSERT_EQ(pool.freeCount(),     10ULL);
}

// ---------------------------------------------------------------------------
// Auto-growth beyond initial slab
// ---------------------------------------------------------------------------
static void test_pool_grows_when_exhausted() {
    MemoryPool<Order> pool(4);  // only 4 slots initially

    std::vector<Order*> ptrs;
    for (int i = 0; i < 8; ++i) {
        Order* p = pool.allocate(static_cast<uint64_t>(i), 0LL,
                                  Side::BUY, OrderKind::LIMIT, 1000000LL, 1ULL);
        ASSERT(p != nullptr);
        ptrs.push_back(p);
    }
    ASSERT(pool.totalCapacity() >= 8ULL);
    for (auto p : ptrs) pool.deallocate(p);
    ASSERT_EQ(pool.freeCount(), pool.totalCapacity());
}

// ---------------------------------------------------------------------------
// Free-list reuse: addresses returned to pool are re-used
// ---------------------------------------------------------------------------
static void test_deallocated_block_is_reused() {
    MemoryPool<Order> pool(8);

    Order* a = pool.allocate(1ULL, 0LL, Side::BUY, OrderKind::LIMIT, 1000000LL, 10ULL);
    void*  addr_a = static_cast<void*>(a);
    pool.deallocate(a);

    // The pool should hand the same slot back immediately (LIFO free list)
    Order* b = pool.allocate(2ULL, 0LL, Side::SELL, OrderKind::LIMIT, 2000000LL, 20ULL);
    ASSERT_EQ(static_cast<void*>(b), addr_a);
    pool.deallocate(b);
}

// ---------------------------------------------------------------------------
// PoolDeleter — shared_ptr integration
// ---------------------------------------------------------------------------
static void test_pool_deleter_returns_to_pool() {
    MemoryPool<Order> pool(8);
    size_t free_before = pool.freeCount();

    {
        Order* raw = pool.allocate(99ULL, 0LL, Side::BUY, OrderKind::LIMIT, 1000000LL, 5ULL);
        auto   sp  = std::shared_ptr<Order>(raw, PoolDeleter<Order>(&pool));
        ASSERT_EQ(pool.freeCount(), free_before - 1);
        // sp goes out of scope here, deleter runs
    }
    ASSERT_EQ(pool.freeCount(), free_before);
}

// ---------------------------------------------------------------------------
// Concurrent allocation / deallocation
// ---------------------------------------------------------------------------
static void test_concurrent_alloc_dealloc() {
    MemoryPool<Order> pool(256);
    constexpr int N_THREADS = 4;
    constexpr int N_OPS     = 64;

    std::vector<std::thread> threads;
    for (int t = 0; t < N_THREADS; ++t) {
        threads.emplace_back([&pool, t]() {
            std::vector<Order*> local;
            local.reserve(N_OPS);
            for (int i = 0; i < N_OPS; ++i) {
                Order* p = pool.allocate(static_cast<uint64_t>(t * 1000 + i),
                                         0LL, Side::BUY, OrderKind::LIMIT,
                                         1000000LL, static_cast<uint64_t>(i + 1));
                local.push_back(p);
            }
            for (auto p : local) pool.deallocate(p);
        });
    }
    for (auto& t : threads) t.join();

    // All slots should be returned
    ASSERT_EQ(pool.freeCount(), pool.totalCapacity());
}

// ---------------------------------------------------------------------------
// All allocated addresses are distinct (no aliasing)
// ---------------------------------------------------------------------------
static void test_no_address_aliasing() {
    MemoryPool<Order> pool(32);
    std::set<void*> addrs;

    std::vector<Order*> ptrs;
    for (int i = 0; i < 32; ++i) {
        Order* p = pool.allocate(static_cast<uint64_t>(i), 0LL,
                                  Side::BUY, OrderKind::LIMIT, 1000000LL, 1ULL);
        ASSERT(addrs.find(static_cast<void*>(p)) == addrs.end());
        addrs.insert(static_cast<void*>(p));
        ptrs.push_back(p);
    }
    for (auto p : ptrs) pool.deallocate(p);
}

// ─── runner ───────────────────────────────────────────────────────────────────

void run_memory_pool_tests() {
    RUN_TEST(test_allocate_returns_non_null);
    RUN_TEST(test_allocated_object_is_valid);
    RUN_TEST(test_capacity_reported_correctly);
    RUN_TEST(test_pool_grows_when_exhausted);
    RUN_TEST(test_deallocated_block_is_reused);
    RUN_TEST(test_pool_deleter_returns_to_pool);
    RUN_TEST(test_concurrent_alloc_dealloc);
    RUN_TEST(test_no_address_aliasing);
}
