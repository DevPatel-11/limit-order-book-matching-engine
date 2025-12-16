#include "order.h"
#include <iostream>
#include <iomanip>

Order::Order(uint64_t id, int64_t timestamp_us, Side side, OrderKind kind,
             int64_t price, uint64_t qty)
    : id_(id)
    , timestamp_us_(timestamp_us)
    , side_(side)
    , kind_(kind)
    , price_(price)
    , quantity_(qty)
    , leaves_qty_(qty)
    , status_(OrderStatus::ACTIVE)
{}

void Order::fill(uint64_t qty) noexcept {
    if (qty >= leaves_qty_) {
        leaves_qty_ = 0;
        status_     = OrderStatus::FILLED;
    } else {
        leaves_qty_ -= qty;
        status_     = OrderStatus::PARTIAL;
    }
}

void Order::cancel() noexcept {
    leaves_qty_ = 0;
    status_     = OrderStatus::CANCELLED;
}

void Order::print() const {
    std::cout << "[Order #" << id_
              << " | " << (side_ == Side::BUY ? "BUY" : "SELL")
              << " | " << (kind_ == OrderKind::LIMIT ? "LIMIT" : "MARKET")
              << " | $" << std::fixed << std::setprecision(2)
              << (static_cast<double>(price_) / static_cast<double>(PRICE_SCALE))
              << " | qty=" << quantity_
              << " leaves=" << leaves_qty_
              << "]\n";
}
