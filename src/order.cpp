#include "order.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

// Regular constructor
Order::Order(uint64_t id, uint64_t ts, double p, uint64_t qty, OrderType t, OrderSide s)
    : order_id(id), timestamp(ts), price(p), quantity(qty), 
      remaining_quantity(qty), type(t), side(s), is_iceberg(false),
      display_quantity(0), hidden_quantity(0) {}

// Iceberg constructor
Order::Order(uint64_t id, uint64_t ts, double p, uint64_t qty, OrderType t, OrderSide s,
             uint64_t display_qty)
    : order_id(id), timestamp(ts), price(p), quantity(qty),
      remaining_quantity(qty), type(t), side(s), is_iceberg(true),
      display_quantity(std::min(display_qty, qty)),
      hidden_quantity(qty > display_qty ? qty - display_qty : 0) {}

uint64_t Order::getVisibleQuantity() const {
    if (!is_iceberg) {
        return remaining_quantity;
    }
    // For iceberg, visible is the current display amount
    return std::min(display_quantity, remaining_quantity);
}

void Order::fill(uint64_t qty) {
    if (qty > remaining_quantity) {
        remaining_quantity = 0;
    } else {
        remaining_quantity -= qty;
    }
    
    // For iceberg orders, update visible portion
    if (is_iceberg && display_quantity > 0) {
        if (qty >= display_quantity) {
            display_quantity = 0;
        } else {
            display_quantity -= qty;
        }
    }
}

void Order::replenishDisplay() {
    if (!is_iceberg || hidden_quantity == 0) {
        return;
    }
    
    // Calculate original display size from constructor
    uint64_t original_display = quantity - hidden_quantity;
    
    // Replenish display from hidden quantity
    uint64_t replenish_amount = std::min(original_display, hidden_quantity);
    display_quantity = std::min(replenish_amount, remaining_quantity);
    hidden_quantity -= replenish_amount;
}

void Order::print() const {
    std::cout << "Order[" << order_id << "] ";
    std::cout << (side == OrderSide::BUY ? "BUY " : "SELL ");
    
    if (is_iceberg) {
        std::cout << getVisibleQuantity() << "/" << remaining_quantity
                  << "(hidden: " << hidden_quantity << ")";
    } else {
        std::cout << remaining_quantity;
    }
    
    std::cout << "@" << std::fixed << std::setprecision(2) << price;
    std::cout << " (filled: " << (quantity - remaining_quantity) << "/" << quantity << ")";
    
    if (is_iceberg) {
        std::cout << " [ICEBERG]";
    }
    
    std::cout << std::endl;
}
