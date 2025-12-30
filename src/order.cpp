#include "order.h"
#include <algorithm>
#include <iomanip>
#include <iostream>

Order::Order(uint64_t id, int64_t timestamp_us, Side side, OrderKind kind,
             int64_t price, uint64_t qty)
    : id_(id), timestamp_us_(timestamp_us), side_(side), kind_(kind),
      price_(price), quantity_(qty), leaves_qty_(qty), status_(OrderStatus::ACTIVE)
{}

Order::Order(uint64_t id, int64_t timestamp_us, Side side,
             int64_t price, uint64_t total_qty, uint64_t display_qty)
    : id_(id), timestamp_us_(timestamp_us), side_(side), kind_(OrderKind::ICEBERG),
      price_(price), quantity_(total_qty), leaves_qty_(total_qty),
      status_(OrderStatus::ACTIVE),
      display_qty_(display_qty), hidden_qty_(total_qty - display_qty),
      orig_display_qty_(display_qty)
{}

Order::Order(uint64_t id, int64_t timestamp_us, Side side,
             int64_t trigger_price, int64_t limit_price, uint64_t qty)
    : id_(id), timestamp_us_(timestamp_us), side_(side), kind_(OrderKind::STOP_LOSS),
      price_(limit_price), quantity_(qty), leaves_qty_(qty),
      status_(OrderStatus::ACTIVE),
      trigger_price_(trigger_price), triggered_(false)
{}

uint64_t Order::visibleQty() const noexcept {
    return isIceberg() ? display_qty_ : leaves_qty_;
}

void Order::replenish() noexcept {
    if (!isIceberg() || hidden_qty_ == 0) return;
    uint64_t lot = std::min(hidden_qty_, orig_display_qty_);
    display_qty_ += lot;
    hidden_qty_  -= lot;
}

void Order::fill(uint64_t qty) noexcept {
    if (qty >= leaves_qty_) {
        leaves_qty_ = 0;
        if (isIceberg()) { display_qty_ = 0; hidden_qty_ = 0; }
        status_ = OrderStatus::FILLED;
    } else {
        leaves_qty_ -= qty;
        if (isIceberg()) {
            uint64_t from_display = std::min(qty, display_qty_);
            display_qty_ -= from_display;
        }
        status_ = OrderStatus::PARTIAL;
    }
}

void Order::cancel() noexcept {
    leaves_qty_ = 0;
    status_     = OrderStatus::CANCELLED;
}

void Order::print() const {
    static const char* kind_names[] = {"LIMIT", "MARKET", "ICEBERG", "STOP_LOSS"};
    std::cout << "[Order #" << id_
              << " | " << (side_ == Side::BUY ? "BUY" : "SELL")
              << " | " << kind_names[static_cast<int>(kind_)]
              << " | $" << std::fixed << std::setprecision(2)
              << price_ / static_cast<double>(PRICE_SCALE)
              << " | qty=" << quantity_
              << " leaves=" << leaves_qty_;
    if (isIceberg())
        std::cout << " (visible=" << display_qty_
                  << " hidden=" << hidden_qty_ << ")";
    std::cout << "]\n";
}
