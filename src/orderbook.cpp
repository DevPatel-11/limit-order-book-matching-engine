#include "orderbook.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <mutex>

void OrderBook::addToBook(OrderPtr order) {
    if (order->side() == Side::BUY) {
        bids_[order->price()].push_back(order);
    } else {
        asks_[order->price()].push_back(order);
    }
    active_[order->id()] = order;
}

bool OrderBook::cancelLocked(uint64_t order_id) {
    auto it = active_.find(order_id);
    if (it == active_.end()) return false;

    OrderPtr order = it->second;
    int64_t  price = order->price();

    auto removeFromLevel = [&](auto& side_map) {
        auto map_it = side_map.find(price);
        if (map_it == side_map.end()) return;
        Level& level = map_it->second;
        level.erase(std::remove_if(level.begin(), level.end(),
            [order_id](const OrderPtr& o) { return o->id() == order_id; }),
            level.end());
        if (level.empty()) side_map.erase(map_it);
    };

    if (order->side() == Side::BUY) removeFromLevel(bids_);
    else                            removeFromLevel(asks_);

    order->cancel();
    active_.erase(it);
    return true;
}


Trade OrderBook::makeTrade(uint64_t buy_id, uint64_t sell_id,
                            int64_t price, uint64_t qty, int64_t ts) {
    return Trade{next_trade_id_++, buy_id, sell_id, price, qty, ts};
}

void OrderBook::checkStopTriggers(int64_t last_price) {
    auto it = pending_stops_.begin();
    while (it != pending_stops_.end()) {
        OrderPtr& stop = *it;
        bool fire = (stop->side() == Side::SELL && last_price <= stop->triggerPrice())
                 || (stop->side() == Side::BUY  && last_price >= stop->triggerPrice());
        if (fire) {
            stop->trigger();
            triggered_stops_.push_back(stop);   // processed after current match cycle
            it = pending_stops_.erase(it);
        } else {
            ++it;
        }
    }
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

            uint64_t available = resting->isIceberg()
                                 ? resting->visibleQty()
                                 : resting->leaves();
            if (available == 0) { level.pop_front(); continue; }

            uint64_t qty   = std::min(order->leaves(), available);
            int64_t  price = it->first;

            order->fill(qty);
            resting->fill(qty);

            Trade t = makeTrade(order->id(), resting->id(),
                                price, qty, order->timestamp());
            result.push_back(t);
            trades_.push_back(t);
            checkStopTriggers(price);

            if (resting->isFilled()) {
                active_.erase(resting->id());
                level.pop_front();
            } else if (resting->isIceberg() && resting->visibleQty() == 0) {
                resting->replenish();   // refill display lot; order stays in queue
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

        if (!order->isMarket() && it->first < order->price()) break;

        Level& level = it->second;
        while (order->isActive() && !level.empty()) {
            OrderPtr resting = level.front();

            uint64_t available = resting->isIceberg()
                                 ? resting->visibleQty()
                                 : resting->leaves();
            if (available == 0) { level.pop_front(); continue; }

            uint64_t qty   = std::min(order->leaves(), available);
            int64_t  price = it->first;

            order->fill(qty);
            resting->fill(qty);

            Trade t = makeTrade(resting->id(), order->id(),
                                price, qty, order->timestamp());
            result.push_back(t);
            trades_.push_back(t);
            checkStopTriggers(price);

            if (resting->isFilled()) {
                active_.erase(resting->id());
                level.pop_front();
            } else if (resting->isIceberg() && resting->visibleQty() == 0) {
                resting->replenish();
            }
        }

        if (level.empty()) bids_.erase(it);
    }

    if (!order->isFilled() && !order->isMarket()) addToBook(order);

    return result;
}

std::vector<Trade> OrderBook::match(OrderPtr order) {
    std::unique_lock lock(mutex_);

    if (order->isStopLoss() && !order->isTriggered()) {
        pending_stops_.push_back(order);
        return {};
    }

    std::vector<Trade> all_trades =
        (order->side() == Side::BUY) ? matchBuy(order) : matchSell(order);

    // Triggered stops are collected inside checkStopTriggers; run them now
    while (!triggered_stops_.empty()) {
        auto batch = std::move(triggered_stops_);
        triggered_stops_.clear();
        for (auto& stop : batch) {
            auto t = (stop->side() == Side::BUY) ? matchBuy(stop) : matchSell(stop);
            all_trades.insert(all_trades.end(), t.begin(), t.end());
        }
    }

    return all_trades;
}

bool OrderBook::cancelOrder(uint64_t order_id) {
    std::unique_lock lock(mutex_);
    return cancelLocked(order_id);
}

bool OrderBook::modifyOrder(uint64_t order_id, int64_t new_price,
                            uint64_t new_qty, int64_t new_timestamp_us) {
    std::unique_lock lock(mutex_);
    auto it = active_.find(order_id);
    if (it == active_.end()) return false;

    OrderPtr old_order = it->second;
    OrderPtr new_order = std::make_shared<Order>(
        old_order->id(), new_timestamp_us,
        old_order->side(), old_order->kind(),
        new_price, new_qty
    );

    cancelLocked(order_id);
    addToBook(new_order);
    return true;
}

int64_t OrderBook::bestBid() const {
    std::shared_lock lock(mutex_);
    return bids_.empty() ? 0 : bids_.begin()->first;
}

int64_t OrderBook::bestAsk() const {
    std::shared_lock lock(mutex_);
    return asks_.empty() ? 0 : asks_.begin()->first;
}

int64_t OrderBook::spread() const {
    std::shared_lock lock(mutex_);
    if (bids_.empty() || asks_.empty()) return 0;
    return asks_.begin()->first - bids_.begin()->first;
}

size_t OrderBook::bidLevels()    const { std::shared_lock l(mutex_); return bids_.size(); }
size_t OrderBook::askLevels()    const { std::shared_lock l(mutex_); return asks_.size(); }
size_t OrderBook::activeOrders() const { std::shared_lock l(mutex_); return active_.size(); }

void OrderBook::printBook(int levels) const {
    std::shared_lock lock(mutex_);
    std::cout << "\n===== ORDER BOOK =====\n"
              << std::fixed << std::setprecision(2);

    // Collect the `levels` ask prices closest to mid (ascending), then
    // display them descending (highest far from mid at top of the ask block).
    std::vector<std::pair<int64_t, uint64_t>> ask_rows;
    ask_rows.reserve(static_cast<size_t>(levels));
    for (auto& [price, level] : asks_) {
        if (static_cast<int>(ask_rows.size()) >= levels) break;
        uint64_t total = 0;
        for (auto& o : level) total += o->visibleQty();
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
        for (auto& o : level) total += o->visibleQty();
        std::cout << "    $" << std::setw(9)
                  << (static_cast<double>(price) / PRICE_SCALE)
                  << "  x  " << total << "\n";
    }

    std::cout << "======================\n\n";
}

void OrderBook::printTrades() const {
    std::shared_lock lock(mutex_);
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

void OrderBook::printDepth(int levels) const {
    std::shared_lock lock(mutex_);
    std::cout << std::fixed << std::setprecision(2);

    // Collect ask levels (ascending in the map = best ask first)
    using Row = std::pair<int64_t, uint64_t>;
    std::vector<Row> ask_rows, bid_rows;

    for (auto& [price, level] : asks_) {
        if (static_cast<int>(ask_rows.size()) >= levels) break;
        uint64_t total = 0;
        for (auto& o : level) total += o->visibleQty();
        ask_rows.push_back({price, total});
    }
    for (auto& [price, level] : bids_) {
        if (static_cast<int>(bid_rows.size()) >= levels) break;
        uint64_t total = 0;
        for (auto& o : level) total += o->visibleQty();
        bid_rows.push_back({price, total});
    }

    std::cout << "\n=== Market Depth (Top " << levels << " Levels) ===\n";
    std::cout << std::setw(12) << "Price"
              << std::setw(10) << "Qty"
              << std::setw(14) << "Cumulative" << "\n";
    std::cout << std::string(36, '-') << "\n";

    // Asks: display highest-to-lowest (furthest from spread first)
    // cumulative counts from best ask outward
    std::cout << "ASKS:\n";
    uint64_t cum = 0;
    for (auto& [p, q] : ask_rows) cum += q;
    for (auto it = ask_rows.rbegin(); it != ask_rows.rend(); ++it) {
        std::cout << std::setw(12) << it->first / static_cast<double>(PRICE_SCALE)   // FIX: consistent scaling
                  << std::setw(10) << it->second
                  << std::setw(14) << cum << "\n";
        cum -= it->second;
    }

    if (!bids_.empty() && !asks_.empty()) {
        double spr = (asks_.begin()->first - bids_.begin()->first)
                     / static_cast<double>(PRICE_SCALE);
        std::cout << "--- spread: $" << spr << " ---\n";
    }

    std::cout << "BIDS:\n";
    cum = 0;
    for (auto& [p, q] : bid_rows) {
        cum += q;
        std::cout << std::setw(12) << p / static_cast<double>(PRICE_SCALE)            // FIX: consistent scaling
                  << std::setw(10) << q
                  << std::setw(14) << cum << "\n";
    }
    std::cout << std::string(36, '=') << "\n";
}
