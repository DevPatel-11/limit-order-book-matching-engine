#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <cstdint>

enum class OrderType {
    LIMIT_BUY,
    LIMIT_SELL,
    MARKET_BUY,
    MARKET_SELL,
    ICEBERG_BUY,
    ICEBERG_SELL,
    STOP_LOSS_BUY,
    STOP_LOSS_SELL
};

enum class OrderSide {
    BUY,
    SELL
};

class Order {
private:
    uint64_t order_id;
    uint64_t timestamp;
    int64_t price;
    uint64_t quantity;
    uint64_t remaining_quantity;
    OrderType type;
    OrderSide side;
    
    // Iceberg order fields
    bool is_iceberg;
    uint64_t display_quantity;  // Visible portion
    uint64_t hidden_quantity;   // Hidden portion

    // Stop-loss order fields
    bool is_stop_loss;
    int64_t trigger_price;  // Price that triggers the stop-loss
    bool is_triggered;     // Whether stop-loss has been triggered

public:
    Order(uint64_t id, uint64_t ts, int64_t p, uint64_t qty, OrderType t, OrderSide s);
    
    // Iceberg constructor
    Order(uint64_t id, uint64_t ts, int64_t p, uint64_t qty, OrderType t, OrderSide s,
          uint64_t display_qty);
    
    // Getters

    // Stop-loss constructor
    Order(uint64_t id, uint64_t ts, int64_t p, uint64_t qty, OrderType t, OrderSide s,
          int64_t trigger_p);
    uint64_t getOrderId() const { return order_id; }
    uint64_t getTimestamp() const { return timestamp; }
    int64_t getPrice() const { return price; }
    uint64_t getQuantity() const { return quantity; }
    uint64_t getRemainingQuantity() const { return remaining_quantity; }
    OrderType getType() const { return type; }
    OrderSide getSide() const { return side; }
    
    // Iceberg getters
    bool isIceberg() const { return is_iceberg; }
    uint64_t getDisplayQuantity() const { return display_quantity; }
    uint64_t getHiddenQuantity() const { return hidden_quantity; }
    uint64_t getVisibleQuantity() const;
    
    // Execution
    void fill(uint64_t qty);
    bool isFilled() const { return remaining_quantity == 0; }
    void replenishDisplay();  // Show more quantity from hidden portion

    // Stop-loss methods
    bool isStopLoss() const { return is_stop_loss; }
    int64_t getTriggerPrice() const { return trigger_price; }
    bool isTriggered() const { return is_triggered; }
    void trigger() { is_triggered = true; }  // Mark stop-loss as triggered
    
    // Display
    void print() const;
};

#endif
