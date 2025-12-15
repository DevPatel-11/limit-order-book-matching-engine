#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <cstdint>

enum class OrderType {
    LIMIT_BUY,
    LIMIT_SELL,
    MARKET_BUY,
    MARKET_SELL
};

enum class OrderSide {
    BUY,
    SELL
};

class Order {
private:
    uint64_t order_id;
    uint64_t timestamp;
    double price;
    uint64_t quantity;
    uint64_t remaining_quantity;
    OrderType type;
    OrderSide side;

public:
    Order(uint64_t id, uint64_t ts, double p, uint64_t qty, OrderType t, OrderSide s);
    
    // Getters
    uint64_t getOrderId() const { return order_id; }
    uint64_t getTimestamp() const { return timestamp; }
    double getPrice() const { return price; }
    uint64_t getQuantity() const { return quantity; }
    uint64_t getRemainingQuantity() const { return remaining_quantity; }
    OrderType getType() const { return type; }
    OrderSide getSide() const { return side; }
    
    // Execution
    void fill(uint64_t qty);
    bool isFilled() const { return remaining_quantity == 0; }
    
    // Display
    void print() const;
};

#endif
