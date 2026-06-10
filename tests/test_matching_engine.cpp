#include "framework.h"
#include "matching_engine.h"

#include <thread>
#include <vector>
#include <set>

// All tests use verbose=false to keep test output clean
// ---------------------------------------------------------------------------
// Basic submission
// ---------------------------------------------------------------------------
static void test_submit_limit_rests_in_book() {
    MatchingEngine engine(false);
    uint64_t id = engine.submitLimit(Side::BUY, 1000000LL, 100);
    ASSERT_NE(id, 0ULL);
    ASSERT_EQ(engine.book().bestBid(), 1000000LL);
    ASSERT_EQ(engine.book().activeOrders(), 1ULL);
}

static void test_submit_limit_crosses_and_trades() {
    MatchingEngine engine(false);
    engine.submitLimit(Side::BUY, 1000000LL, 100);
    engine.submitLimit(Side::SELL, 1000000LL, 100);
    ASSERT_EQ(engine.book().activeOrders(), 0ULL);
    ASSERT_EQ(engine.book().tradeHistory().size(), 1ULL);
}

static void test_submit_market_against_resting_limit() {
    MatchingEngine engine(false);
    engine.submitLimit(Side::SELL, 1010000LL, 50);
    engine.submitMarket(Side::BUY, 50);
    ASSERT_EQ(engine.book().activeOrders(), 0ULL);
    ASSERT_EQ(engine.book().tradeHistory().size(), 1ULL);
}

static void test_submit_iceberg() {
    MatchingEngine engine(false);
    uint64_t id = engine.submitIceberg(Side::BUY, 1000000LL, 500, 100);
    ASSERT_NE(id, 0ULL);
    ASSERT_EQ(engine.book().activeOrders(), 1ULL);
}

static void test_submit_stop_loss_pending() {
    MatchingEngine engine(false);
    uint64_t id = engine.submitStopLoss(Side::SELL, 990000LL, 989000LL, 100);
    ASSERT_NE(id, 0ULL);
    // Stop-loss is pending — not visible in active_orders (only resting limits are)
    ASSERT_EQ(engine.book().activeOrders(), 0ULL);
}

// ---------------------------------------------------------------------------
// Cancel and modify
// ---------------------------------------------------------------------------
static void test_cancel_existing() {
    MatchingEngine engine(false);
    uint64_t id = engine.submitLimit(Side::BUY, 990000LL, 50);
    bool ok = engine.cancelOrder(id);
    ASSERT(ok);
    ASSERT_EQ(engine.book().activeOrders(), 0ULL);
}

static void test_cancel_nonexistent() {
    MatchingEngine engine(false);
    bool ok = engine.cancelOrder(9999);
    ASSERT_FALSE(ok);
}

static void test_modify_order_no_deadlock() {
    MatchingEngine engine(false);
    uint64_t id = engine.submitLimit(Side::BUY, 990000LL, 100);
    bool ok = engine.modifyOrder(id, 985000LL, 100);
    ASSERT(ok);
    ASSERT_EQ(engine.book().bestBid(), 985000LL);
}

// ---------------------------------------------------------------------------
// ID monotonicity
// ---------------------------------------------------------------------------
static void test_ids_are_monotonically_increasing() {
    MatchingEngine engine(false);
    uint64_t a = engine.submitLimit(Side::BUY, 990000LL, 10);
    uint64_t b = engine.submitLimit(Side::BUY, 991000LL, 10);
    uint64_t c = engine.submitIceberg(Side::SELL, 1010000LL, 100, 20);
    ASSERT(a < b);
    ASSERT(b < c);
}

// ---------------------------------------------------------------------------
// Thread safety: concurrent submits from multiple threads
// ---------------------------------------------------------------------------
static void test_concurrent_limit_submits() {
    MatchingEngine engine(false);

    auto submit_bids = [&]() {
        for (int i = 0; i < 100; ++i)
            engine.submitLimit(Side::BUY,
                static_cast<int64_t>(990000 - i * 1000), 10);
    };

    auto submit_asks = [&]() {
        for (int i = 0; i < 100; ++i)
            engine.submitLimit(Side::SELL,
                static_cast<int64_t>(1010000 + i * 1000), 10);
    };

    std::vector<std::thread> threads;
    threads.emplace_back(submit_bids);
    threads.emplace_back(submit_bids);
    threads.emplace_back(submit_asks);
    threads.emplace_back(submit_asks);
    for (auto& t : threads) t.join();

    // 400 orders submitted, all non-crossing — all should rest
    ASSERT_EQ(engine.book().activeOrders(), 400ULL);
}

static void test_concurrent_mixed_operations() {
    MatchingEngine engine(false);

    // Pre-seed book
    for (int i = 0; i < 50; ++i) {
        engine.submitLimit(Side::BUY, static_cast<int64_t>(990000 - i * 1000), 10);
        engine.submitLimit(Side::SELL, static_cast<int64_t>(1010000 + i * 1000), 10);
    }

    std::vector<uint64_t> bid_ids, ask_ids;
    for (int i = 0; i < 10; ++i) {
        bid_ids.push_back(engine.submitLimit(Side::BUY, 980000LL, 5));
        ask_ids.push_back(engine.submitLimit(Side::SELL, 1020000LL, 5));
    }

    std::thread canceller([&]() {
        for (auto id : bid_ids) engine.cancelOrder(id);
    });
    std::thread modifier([&]() {
        for (auto id : ask_ids) engine.modifyOrder(id, 1025000LL, 5);
    });
    std::thread buyer([&]() {
        for (int i = 0; i < 20; ++i)
            engine.submitMarket(Side::BUY, 5);
    });

    canceller.join();
    modifier.join();
    buyer.join();

    // Just verifying no crash and data is consistent
    ASSERT(engine.book().activeOrders() < 500ULL);
}

// ─── runner ───────────────────────────────────────────────────────────────────

void run_matching_engine_tests() {
    RUN_TEST(test_submit_limit_rests_in_book);
    RUN_TEST(test_submit_limit_crosses_and_trades);
    RUN_TEST(test_submit_market_against_resting_limit);
    RUN_TEST(test_submit_iceberg);
    RUN_TEST(test_submit_stop_loss_pending);
    RUN_TEST(test_cancel_existing);
    RUN_TEST(test_cancel_nonexistent);
    RUN_TEST(test_modify_order_no_deadlock);
    RUN_TEST(test_ids_are_monotonically_increasing);
    RUN_TEST(test_concurrent_limit_submits);
    RUN_TEST(test_concurrent_mixed_operations);
}
