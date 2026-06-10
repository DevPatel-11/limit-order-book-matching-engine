#include "framework.h"
#include "orderbook.h"

// Helper: create a limit OrderPtr directly (used for low-level book tests)
static OrderPtr makeLimitOrder(uint64_t id, Side side, int64_t price, uint64_t qty) {
    return std::make_shared<Order>(id, static_cast<int64_t>(id), side,
                                   OrderKind::LIMIT, price, qty);
}

static OrderPtr makeMarketOrder(uint64_t id, Side side, uint64_t qty) {
    return std::make_shared<Order>(id, static_cast<int64_t>(id), side,
                                   OrderKind::MARKET, 0LL, qty);
}

static OrderPtr makeIceberg(uint64_t id, Side side, int64_t price,
                             uint64_t total, uint64_t display) {
    return std::make_shared<Order>(id, static_cast<int64_t>(id), side,
                                   price, total, display);
}

static OrderPtr makeStopLoss(uint64_t id, Side side,
                              int64_t trigger, int64_t limit, uint64_t qty) {
    return std::make_shared<Order>(id, static_cast<int64_t>(id), side,
                                   trigger, limit, qty);
}

// ---------------------------------------------------------------------------
// Basic bid / ask placement
// ---------------------------------------------------------------------------
static void test_resting_bid_no_cross() {
    OrderBook book;
    auto o = makeLimitOrder(1, Side::BUY, 1000000LL, 100);
    auto trades = book.match(o);
    ASSERT(trades.empty());
    ASSERT_EQ(book.bestBid(), 1000000LL);
    ASSERT_EQ(book.bestAsk(), 0LL);
    ASSERT_EQ(book.activeOrders(), 1ULL);
}

static void test_resting_ask_no_cross() {
    OrderBook book;
    auto o = makeLimitOrder(1, Side::SELL, 1010000LL, 50);
    auto trades = book.match(o);
    ASSERT(trades.empty());
    ASSERT_EQ(book.bestAsk(), 1010000LL);
    ASSERT_EQ(book.bestBid(), 0LL);
}

// ---------------------------------------------------------------------------
// Price-time (FIFO) matching
// ---------------------------------------------------------------------------
static void test_simple_cross_full_fill() {
    OrderBook book;
    book.match(makeLimitOrder(1, Side::BUY, 1000000LL, 100));
    auto trades = book.match(makeLimitOrder(2, Side::SELL, 1000000LL, 100));

    ASSERT_EQ(trades.size(), 1ULL);
    ASSERT_EQ(trades[0].quantity, 100ULL);
    ASSERT_EQ(trades[0].price, 1000000LL);
    ASSERT_EQ(trades[0].buy_order_id, 1ULL);
    ASSERT_EQ(trades[0].sell_order_id, 2ULL);
    ASSERT_EQ(book.activeOrders(), 0ULL);
}

static void test_partial_fill_leaves_resting() {
    OrderBook book;
    book.match(makeLimitOrder(1, Side::BUY, 1000000LL, 100));
    auto trades = book.match(makeLimitOrder(2, Side::SELL, 1000000LL, 60));

    ASSERT_EQ(trades.size(), 1ULL);
    ASSERT_EQ(trades[0].quantity, 60ULL);
    ASSERT_EQ(book.activeOrders(), 1ULL);   // 40 remaining bid
}

static void test_fifo_priority_two_bids_same_price() {
    OrderBook book;
    book.match(makeLimitOrder(1, Side::BUY, 1000000LL, 50));   // earlier
    book.match(makeLimitOrder(2, Side::BUY, 1000000LL, 50));   // later

    // Sell 50 — should match with order 1 first (FIFO)
    auto trades = book.match(makeLimitOrder(3, Side::SELL, 1000000LL, 50));
    ASSERT_EQ(trades.size(), 1ULL);
    ASSERT_EQ(trades[0].buy_order_id, 1ULL);   // first bid was matched
    ASSERT_EQ(book.activeOrders(), 1ULL);       // order 2 remains
}

static void test_market_buy_takes_best_ask() {
    OrderBook book;
    book.match(makeLimitOrder(1, Side::SELL, 1010000LL, 100));
    book.match(makeLimitOrder(2, Side::SELL, 1020000LL, 100));

    auto trades = book.match(makeMarketOrder(3, Side::BUY, 80));
    ASSERT_EQ(trades.size(), 1ULL);
    ASSERT_EQ(trades[0].price, 1010000LL);  // best ask first
    ASSERT_EQ(trades[0].quantity, 80ULL);
}

static void test_market_buy_sweeps_multiple_levels() {
    OrderBook book;
    book.match(makeLimitOrder(1, Side::SELL, 1010000LL, 40));
    book.match(makeLimitOrder(2, Side::SELL, 1020000LL, 40));

    auto trades = book.match(makeMarketOrder(3, Side::BUY, 80));
    ASSERT_EQ(trades.size(), 2ULL);
    ASSERT_EQ(book.activeOrders(), 0ULL);
}

// ---------------------------------------------------------------------------
// Cancel order
// ---------------------------------------------------------------------------
static void test_cancel_existing_order() {
    OrderBook book;
    book.match(makeLimitOrder(1, Side::BUY, 1000000LL, 100));
    bool ok = book.cancelOrder(1);
    ASSERT(ok);
    ASSERT_EQ(book.activeOrders(), 0ULL);
    ASSERT_EQ(book.bestBid(), 0LL);
}

static void test_cancel_nonexistent_order() {
    OrderBook book;
    bool ok = book.cancelOrder(999);
    ASSERT_FALSE(ok);
}

// ---------------------------------------------------------------------------
// Modify order (must not deadlock)
// ---------------------------------------------------------------------------
static void test_modify_changes_price() {
    OrderBook book;
    book.match(makeLimitOrder(1, Side::BUY, 1000000LL, 100));
    bool ok = book.modifyOrder(1, 990000LL, 100, 2);
    ASSERT(ok);
    ASSERT_EQ(book.bestBid(), 990000LL);
}

static void test_modify_nonexistent_order() {
    OrderBook book;
    bool ok = book.modifyOrder(999, 1000000LL, 50, 1);
    ASSERT_FALSE(ok);
}

// ---------------------------------------------------------------------------
// Iceberg matching
// ---------------------------------------------------------------------------
static void test_iceberg_shows_only_visible_qty() {
    OrderBook book;
    // Iceberg: total 500, display 100
    auto ice = makeIceberg(1, Side::BUY, 1000000LL, 500, 100);
    book.match(ice);

    // Sell 100 — matches the visible lot, triggers replenishment
    auto trades = book.match(makeLimitOrder(2, Side::SELL, 1000000LL, 100));
    ASSERT_EQ(trades.size(), 1ULL);
    ASSERT_EQ(trades[0].quantity, 100ULL);
    // Iceberg should still be alive (400 remain)
    ASSERT_EQ(book.activeOrders(), 1ULL);
}

static void test_iceberg_fully_consumed() {
    OrderBook book;
    auto ice = makeIceberg(1, Side::BUY, 1000000LL, 200, 100);
    book.match(ice);

    auto trades = book.match(makeLimitOrder(2, Side::SELL, 1000000LL, 200));
    // 200 total: two lots of 100 each
    ASSERT_EQ(book.activeOrders(), 0ULL);
    ASSERT_EQ(book.tradeHistory().size(), 2ULL);
}

// ---------------------------------------------------------------------------
// Stop-loss triggering
// ---------------------------------------------------------------------------
static void test_stop_loss_triggers_on_price() {
    OrderBook book;

    // Resting bid at $99.50
    book.match(makeLimitOrder(1, Side::BUY, 995000LL, 200));

    // Stop-loss sell: trigger ≤ $99.50, limit = $99.40
    auto stop = makeStopLoss(2, Side::SELL, 995000LL, 994000LL, 100);
    auto t0 = book.match(stop);
    ASSERT(t0.empty());                        // not yet triggered
    ASSERT_EQ(book.activeOrders(), 1ULL);      // only the bid is active

    // Limit sell @ $99.50 — executes at $99.50, triggers the stop
    auto t1 = book.match(makeLimitOrder(3, Side::SELL, 995000LL, 50));
    // t1 includes the direct trade + trades from the triggered stop
    ASSERT_FALSE(t1.empty());
}

static void test_stop_loss_not_triggered_above_price() {
    OrderBook book;

    // Resting bid at $100.00
    book.match(makeLimitOrder(1, Side::BUY, 1000000LL, 100));

    // Stop-loss sell triggers at ≤ $99.00 (far below market)
    auto stop = makeStopLoss(2, Side::SELL, 990000LL, 989000LL, 50);
    book.match(stop);

    // Trade happens at $100.00 — above trigger price, stop stays pending
    auto trades = book.match(makeLimitOrder(3, Side::SELL, 1000000LL, 100));
    ASSERT_EQ(trades.size(), 1ULL);             // only the direct trade
    ASSERT_EQ(trades[0].price, 1000000LL);
}

// ---------------------------------------------------------------------------
// Spread and levels
// ---------------------------------------------------------------------------
static void test_spread_calculation() {
    OrderBook book;
    book.match(makeLimitOrder(1, Side::BUY, 990000LL, 10));
    book.match(makeLimitOrder(2, Side::SELL, 1010000LL, 10));
    ASSERT_EQ(book.spread(), 20000LL);
    ASSERT_EQ(book.bidLevels(), 1ULL);
    ASSERT_EQ(book.askLevels(), 1ULL);
}

// ─── runner ───────────────────────────────────────────────────────────────────

void run_orderbook_tests() {
    RUN_TEST(test_resting_bid_no_cross);
    RUN_TEST(test_resting_ask_no_cross);
    RUN_TEST(test_simple_cross_full_fill);
    RUN_TEST(test_partial_fill_leaves_resting);
    RUN_TEST(test_fifo_priority_two_bids_same_price);
    RUN_TEST(test_market_buy_takes_best_ask);
    RUN_TEST(test_market_buy_sweeps_multiple_levels);
    RUN_TEST(test_cancel_existing_order);
    RUN_TEST(test_cancel_nonexistent_order);
    RUN_TEST(test_modify_changes_price);
    RUN_TEST(test_modify_nonexistent_order);
    RUN_TEST(test_iceberg_shows_only_visible_qty);
    RUN_TEST(test_iceberg_fully_consumed);
    RUN_TEST(test_stop_loss_triggers_on_price);
    RUN_TEST(test_stop_loss_not_triggered_above_price);
    RUN_TEST(test_spread_calculation);
}
