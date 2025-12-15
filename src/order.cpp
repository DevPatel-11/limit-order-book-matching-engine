#include "order.h"
#include <iostream>
#include <iomanip>

Order::Order(uint64_t id, uint64_t ts, double p, uint64_t qty, OrderType t, OrderSide s)
    : order_id(id), timestamp(ts), price(p), quantity(qty), 
      remaining_quantity(qty), type(t), side(s) {}

void Order::fill(uint64_t qty) {
    if (qty > remaining_quantity) {
        remaining_quantity = 0;
    } else {
        remaining_quantity -= qty;
    }
}

void Order::print() const {
    std::cout << "Order[" << order_id << "] ";
    std::cout << (side == OrderSide::BUY ? "BUY " : "SELL ");
    std::cout << remaining_quantity << "@" << std::fixed << std::setprecision(2) << price;
    std::cout << " (filled: " << (quantity - remaining_quantity) << "/" << quantity << ")";
    std::cout << std::endl;
}
