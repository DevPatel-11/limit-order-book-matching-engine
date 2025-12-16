#include "orderbook.h"
#include <algorithm>
#include <iomanip>
#include <iostream>

void OrderBook::addToBook(OrderPtr order) {
    if (order->side() == Side::BUY) {
        bids_[order->price()].push_back(order);
    } else {
        asks_[order->price()].push_back(order);
    }
    active_[order->id()] = order;
}

Trade OrderBook::makeTrade(uint64_t buy_id, uint64_t sell_id,
                            int64_t price, uint64_t qty, int64_t ts) {
    return Trade{next_trade_id_++, buy_id, sell_id, price, qty, ts};
}

std::vector<Trade> OrderBook::matchBuy(OrderPtr order) {
    std::vector<Trade> result;

    while (order->isActive() && !asks_.empty()) {
        auto it = asks_.begin();

        // Limit orders may not lift above their stated price.
        if (!order->isMarket() && it->first > order->price()) break;

        Level& level = it->second;
        while (order->isActive() && !level.empty()) {
            OrderPtr resting = level.front();

            uint64_t qty   = std::min(order->leaves(), resting->leaves());
            int64_t  price = resting->price();

            order->fill(qty);
            resting->fill(qty);

            Trade t = makeTrade(order->id(), resting->id(),
                                price, qty, order->timestamp());
            result.push_back(t);
            trades_.push_back(t);

            if (resting->isFilled()) {
                active_.erase(resting->id());
                level.pop_front();
            }
        }

        if (level.empty()) asks_.erase(it);
    }

    if (!order->isFilled() && !order->isMarket()) addToBook(order);

    return result;
}

std::vector<Trade> OrderBook::matchSell(OrderPtr order) {
    std::vector<Trade> result;

    while (order->isActive() && !bids_.empty()) {
        auto it = bids_.begin();

        // Limit orders may not hit below their stated price.
        if (!order->isMarket() && it->first < order->price()) break;

        Level& level = it->second;
        while (order->isActive() && !level.empty()) {
            OrderPtr resting = level.front();

            uint64_t qty   = std::min(order->leaves(), resting->leaves());
            int64_t  price = resting->price();

            order->fill(qty);
            resting->fill(qty);

            Trade t = makeTrade(resting->id(), order->id(),
                                price, qty, order->timestamp());
            result.push_back(t);
            trades_.push_back(t);

            if (resting->isFilled()) {
                active_.erase(resting->id());
                level.pop_front();
            }
        }

        if (level.empty()) bids_.erase(it);
    }

    if (!order->isFilled() && !order->isMarket()) addToBook(order);

    return result;
}

std::vector<Trade> OrderBook::match(OrderPtr order) {
    return order->side() == Side::BUY ? matchBuy(order) : matchSell(order);
}

int64_t OrderBook::bestBid() const {
    return bids_.empty() ? 0 : bids_.begin()->first;
}

int64_t OrderBook::bestAsk() const {
    return asks_.empty() ? 0 : asks_.begin()->first;
}

int64_t OrderBook::spread() const {
    return (bids_.empty() || asks_.empty()) ? 0 : bestAsk() - bestBid();
}

size_t OrderBook::bidLevels()   const { return bids_.size(); }
size_t OrderBook::askLevels()   const { return asks_.size(); }
size_t OrderBook::activeOrders() const { return active_.size(); }

void OrderBook::printBook(int levels) const {
    std::cout << "\n===== ORDER BOOK =====\n"
              << std::fixed << std::setprecision(2);

    // Collect the `levels` ask prices closest to mid (ascending), then
    // display them descending (highest far from mid at top of the ask block).
    std::vector<std::pair<int64_t, uint64_t>> ask_rows;
    ask_rows.reserve(static_cast<size_t>(levels));
    for (auto& [price, level] : asks_) {
        if (static_cast<int>(ask_rows.size()) >= levels) break;
        uint64_t total = 0;
        for (auto& o : level) total += o->leaves();
        ask_rows.emplace_back(price, total);
    }

    std::cout << "  ASKS:\n";
    for (auto it = ask_rows.rbegin(); it != ask_rows.rend(); ++it) {
        std::cout << "    $" << std::setw(9)
                  << (static_cast<double>(it->first) / PRICE_SCALE)
                  << "  x  " << it->second << "\n";
    }

    std::cout << "  -------- spread: $"
              << (static_cast<double>(spread()) / PRICE_SCALE)
              << " --------\n";

    std::cout << "  BIDS:\n";
    int count = 0;
    for (auto& [price, level] : bids_) {
        if (count++ >= levels) break;
        uint64_t total = 0;
        for (auto& o : level) total += o->leaves();
        std::cout << "    $" << std::setw(9)
                  << (static_cast<double>(price) / PRICE_SCALE)
                  << "  x  " << total << "\n";
    }

    std::cout << "======================\n\n";
}

void OrderBook::printTrades() const {
    std::cout << "\n===== TRADE HISTORY (" << trades_.size() << " trade(s)) =====\n"
              << std::fixed << std::setprecision(2);
    for (auto& t : trades_) {
        std::cout << "  Trade #" << t.trade_id
                  << "  buy=" << t.buy_order_id
                  << "  sell=" << t.sell_order_id
                  << "  qty=" << t.quantity
                  << "  @$" << (static_cast<double>(t.price) / PRICE_SCALE)
                  << "\n";
    }
    std::cout << "==========================================\n\n";
}
