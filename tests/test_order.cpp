#include "framework.h"
#include "order.h"

// Helpers to avoid ambiguous-overload errors from integer literals.
// Using typed variables gives exact type matches to the right constructor.
static Order makeIceberg(uint64_t id, int64_t ts, Side side,
                          int64_t price, uint64_t total, uint64_t display) {
    uint64_t t = total, d = display;   // exact uint64_t — selects iceberg ctor
    return Order(id, ts, side, price, t, d);
}
static Order makeStop(uint64_t id, int64_t ts, Side side,
                       int64_t trigger, int64_t limit, uint64_t qty) {
    int64_t tr = trigger, li = limit;  // exact int64_t — selects stop-loss ctor
    return Order(id, ts, side, tr, li, qty);
}

// ---------------------------------------------------------------------------
// LIMIT order
// ---------------------------------------------------------------------------
static void test_limit_order_basic() {
    Order o(1ULL, 0LL, Side::BUY, OrderKind::LIMIT, 1000000LL, 100ULL);
    ASSERT_EQ(o.id(),       1ULL);
    ASSERT_EQ(o.price(),    1000000LL);
    ASSERT_EQ(o.quantity(), 100ULL);
    ASSERT_EQ(o.leaves(),   100ULL);
    ASSERT(o.side() == Side::BUY);
    ASSERT(o.kind() == OrderKind::LIMIT);
    ASSERT_FALSE(o.isFilled());
    ASSERT_FALSE(o.isMarket());
    ASSERT_FALSE(o.isIceberg());
    ASSERT_FALSE(o.isStopLoss());
}

static void test_limit_order_fill_partial() {
    Order o(2ULL, 0LL, Side::SELL, OrderKind::LIMIT, 2000000LL, 50ULL);
    o.fill(20);
    ASSERT_EQ(o.leaves(), 30ULL);
    ASSERT_FALSE(o.isFilled());
}

static void test_limit_order_fill_full() {
    Order o(3ULL, 0LL, Side::BUY, OrderKind::LIMIT, 500000LL, 10ULL);
    o.fill(10);
    ASSERT(o.isFilled());
    ASSERT_EQ(o.leaves(), 0ULL);
}

static void test_limit_order_cancel() {
    Order o(4ULL, 0LL, Side::SELL, OrderKind::LIMIT, 1500000LL, 75ULL);
    o.cancel();
    ASSERT(o.status() == OrderStatus::CANCELLED);
}

// ---------------------------------------------------------------------------
// MARKET order
// ---------------------------------------------------------------------------
static void test_market_order() {
    Order o(5ULL, 0LL, Side::BUY, OrderKind::MARKET, 0LL, 200ULL);
    ASSERT(o.isMarket());
    ASSERT_EQ(o.price(), 0LL);
    ASSERT_EQ(o.leaves(), 200ULL);
}

// ---------------------------------------------------------------------------
// ICEBERG order
// ---------------------------------------------------------------------------
static void test_iceberg_initial_visible() {
    auto o = makeIceberg(6, 0LL, Side::BUY, 1000000LL, 500, 100);
    ASSERT(o.isIceberg());
    ASSERT_EQ(o.visibleQty(), 100ULL);
    ASSERT_EQ(o.leaves(), 500ULL);
}

static void test_iceberg_fill_depletes_display() {
    auto o = makeIceberg(7, 0LL, Side::BUY, 1000000LL, 500, 100);
    o.fill(80);
    ASSERT_EQ(o.visibleQty(), 20ULL);   // 100 - 80
    ASSERT_EQ(o.leaves(), 420ULL);
}

static void test_iceberg_replenish_after_display_exhausted() {
    auto o = makeIceberg(8, 0LL, Side::BUY, 1000000LL, 500, 100);
    o.fill(100);
    ASSERT_EQ(o.visibleQty(), 0ULL);
    ASSERT_FALSE(o.isFilled());

    o.replenish();
    ASSERT_EQ(o.visibleQty(), 100ULL);
    ASSERT_EQ(o.leaves(), 400ULL);
}

static void test_iceberg_replenish_partial_last_lot() {
    auto o = makeIceberg(9, 0LL, Side::BUY, 1000000LL, 250, 100);
    o.fill(100); o.replenish();   // lot 2
    o.fill(100); o.replenish();   // lot 3 — only 50 remain
    ASSERT_EQ(o.visibleQty(), 50ULL);
    ASSERT_EQ(o.leaves(), 50ULL);
}

static void test_iceberg_fully_filled() {
    auto o = makeIceberg(10, 0LL, Side::SELL, 1000000LL, 100, 50);
    o.fill(50); o.replenish();
    o.fill(50);
    ASSERT(o.isFilled());
}

// ---------------------------------------------------------------------------
// STOP-LOSS order
// ---------------------------------------------------------------------------
static void test_stop_loss_initial_state() {
    auto o = makeStop(11, 0LL, Side::SELL, 995000LL, 994000LL, 100);
    ASSERT(o.isStopLoss());
    ASSERT_EQ(o.triggerPrice(), 995000LL);
    ASSERT_EQ(o.price(), 994000LL);
    ASSERT_FALSE(o.isTriggered());
}

static void test_stop_loss_trigger() {
    auto o = makeStop(12, 0LL, Side::SELL, 995000LL, 994000LL, 100);
    o.trigger();
    ASSERT(o.isTriggered());
}

// ─── runner ───────────────────────────────────────────────────────────────────

void run_order_tests() {
    RUN_TEST(test_limit_order_basic);
    RUN_TEST(test_limit_order_fill_partial);
    RUN_TEST(test_limit_order_fill_full);
    RUN_TEST(test_limit_order_cancel);
    RUN_TEST(test_market_order);
    RUN_TEST(test_iceberg_initial_visible);
    RUN_TEST(test_iceberg_fill_depletes_display);
    RUN_TEST(test_iceberg_replenish_after_display_exhausted);
    RUN_TEST(test_iceberg_replenish_partial_last_lot);
    RUN_TEST(test_iceberg_fully_filled);
    RUN_TEST(test_stop_loss_initial_state);
    RUN_TEST(test_stop_loss_trigger);
}
