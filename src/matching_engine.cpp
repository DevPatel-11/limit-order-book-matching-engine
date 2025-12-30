#include "matching_engine.h"
#include <chrono>
#include <iomanip>
#include <iostream>

MatchingEngine::MatchingEngine(bool verbose)
    : pool_(1024), verbose_(verbose)
{}

int64_t MatchingEngine::nowUs() const {
    using namespace std::chrono;
    return duration_cast<microseconds>(
        steady_clock::now().time_since_epoch()
    ).count();
}

void MatchingEngine::logTrades(const std::vector<Trade>& trades) const {
    if (trades.empty()) {
        std::cout << "  No trades matched.\n";
        return;
    }
    std::cout << std::fixed << std::setprecision(2)
              << "  Matched " << trades.size() << " trade(s):\n";
    for (auto& t : trades) {
        std::cout << "    qty=" << t.quantity
                  << " @$" << (static_cast<double>(t.price) / PRICE_SCALE)
                  << "\n";
    }
}

uint64_t MatchingEngine::submitLimit(Side side, int64_t price, uint64_t qty) {
    uint64_t id = nextId();
    int64_t  ts = nowUs();

    if (verbose_) {
        std::cout << "[LIMIT " << (side == Side::BUY ? "BUY " : "SELL")
                  << "] id=" << id
                  << "  price=$" << std::fixed << std::setprecision(2)
                  << (static_cast<double>(price) / PRICE_SCALE)
                  << "  qty=" << qty << "\n";
    }

    Order* raw  = pool_.allocate(id, ts, side, OrderKind::LIMIT, price, qty);
    auto order  = std::shared_ptr<Order>(raw, PoolDeleter<Order>(&pool_));
    auto trades = book_.match(order);

    if (verbose_) logTrades(trades);

    return id;
}

void MatchingEngine::submitMarket(Side side, uint64_t qty) {
    uint64_t id = nextId();
    int64_t  ts = nowUs();

    if (verbose_) {
        std::cout << "[MARKET " << (side == Side::BUY ? "BUY " : "SELL")
                  << "] id=" << id
                  << "  qty=" << qty << "\n";
    }

    Order* raw  = pool_.allocate(id, ts, side, OrderKind::MARKET, 0LL, qty);
    auto order  = std::shared_ptr<Order>(raw, PoolDeleter<Order>(&pool_));
    auto trades = book_.match(order);

    if (verbose_) logTrades(trades);
}

uint64_t MatchingEngine::submitIceberg(Side side, int64_t price,
                                       uint64_t total_qty, uint64_t display_qty) {
    uint64_t id = nextId();
    int64_t  ts = nowUs();

    if (verbose_)
        std::cout << "[ICEBERG " << (side == Side::BUY ? "BUY " : "SELL")
                  << "] id=" << id
                  << "  price=$" << std::fixed << std::setprecision(2)
                  << price / static_cast<double>(PRICE_SCALE)
                  << "  total=" << total_qty << "  visible=" << display_qty << "\n";

    Order* raw  = pool_.allocate(id, ts, side, price, total_qty, display_qty);
    auto order  = std::shared_ptr<Order>(raw, PoolDeleter<Order>(&pool_));
    auto trades = book_.match(order);
    if (verbose_) logTrades(trades);
    return id;
}

bool MatchingEngine::cancelOrder(uint64_t order_id) {
    bool ok = book_.cancelOrder(order_id);
    if (verbose_)
        std::cout << "[CANCEL] #" << order_id
                  << (ok ? " — cancelled\n" : " — not found\n");
    return ok;
}

bool MatchingEngine::modifyOrder(uint64_t order_id, int64_t new_price, uint64_t new_qty) {
    bool ok = book_.modifyOrder(order_id, new_price, new_qty, nowUs());
    if (verbose_)
        std::cout << "[MODIFY] #" << order_id
                  << (ok ? " — modified\n" : " — not found\n");
    return ok;
}

void MatchingEngine::printBook(int levels) const { book_.printBook(levels); }
void MatchingEngine::printTrades()          const { book_.printTrades();    }

void MatchingEngine::printStats() const {
    std::cout << "\n===== ENGINE STATS =====\n"
              << std::fixed << std::setprecision(2);

    std::cout << "  Active orders : " << book_.activeOrders() << "\n"
              << "  Bid levels    : " << book_.bidLevels()    << "\n"
              << "  Ask levels    : " << book_.askLevels()    << "\n";

    if (book_.bestBid() > 0)
        std::cout << "  Best bid      : $"
                  << (static_cast<double>(book_.bestBid()) / PRICE_SCALE) << "\n";

    if (book_.bestAsk() > 0)
        std::cout << "  Best ask      : $"
                  << (static_cast<double>(book_.bestAsk()) / PRICE_SCALE) << "\n";

    if (book_.bestBid() > 0 && book_.bestAsk() > 0)
        std::cout << "  Spread        : $"
                  << (static_cast<double>(book_.spread()) / PRICE_SCALE) << "\n";

    std::cout << "  Total trades  : " << book_.tradeHistory().size() << "\n"
              << "========================\n\n";
}

void MatchingEngine::printPoolStats() const {
    size_t capacity = pool_.totalCapacity();
    size_t free_cnt = pool_.freeCount();
    std::cout << "\n===== MEMORY POOL =====\n"
              << "  Capacity : " << capacity << " slots\n"
              << "  Free     : " << free_cnt << " slots\n"
              << "  In use   : " << (capacity - free_cnt) << " slots\n"
              << "=======================\n\n";
}
