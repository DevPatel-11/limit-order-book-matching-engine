#pragma once

#include <cstdint>

constexpr int64_t PRICE_SCALE = 10000;

enum class Side        : uint8_t { BUY, SELL };
enum class OrderKind   : uint8_t { LIMIT, MARKET, ICEBERG, STOP_LOSS };
enum class OrderStatus : uint8_t { ACTIVE, PARTIAL, FILLED, CANCELLED };

class Order {
public:
    // Limit / Market
    Order(uint64_t id, int64_t timestamp_us, Side side, OrderKind kind,
          int64_t price, uint64_t qty);

    // Iceberg (display_qty < total_qty hides the remainder)
    Order(uint64_t id, int64_t timestamp_us, Side side,
          int64_t price, uint64_t total_qty, uint64_t display_qty);

    // Stop-loss (trigger_price activates a limit order at limit_price)
    Order(uint64_t id, int64_t timestamp_us, Side side,
          int64_t trigger_price, int64_t limit_price, uint64_t qty);

    uint64_t    id()        const noexcept { return id_; }
    int64_t     timestamp() const noexcept { return timestamp_us_; }
    Side        side()      const noexcept { return side_; }
    OrderKind   kind()      const noexcept { return kind_; }
    int64_t     price()     const noexcept { return price_; }
    uint64_t    quantity()  const noexcept { return quantity_; }
    uint64_t    leaves()    const noexcept { return leaves_qty_; }
    OrderStatus status()    const noexcept { return status_; }

    bool isFilled()   const noexcept { return leaves_qty_ == 0; }
    bool isMarket()   const noexcept { return kind_ == OrderKind::MARKET; }
    bool isIceberg()  const noexcept { return kind_ == OrderKind::ICEBERG; }
    bool isStopLoss() const noexcept { return kind_ == OrderKind::STOP_LOSS; }
    bool isActive()   const noexcept {
        return status_ == OrderStatus::ACTIVE || status_ == OrderStatus::PARTIAL;
    }

    // Iceberg helpers
    uint64_t visibleQty() const noexcept;
    void     replenish()  noexcept;

    // Stop-loss helpers
    int64_t triggerPrice() const noexcept { return trigger_price_; }
    bool    isTriggered()  const noexcept { return triggered_; }
    void    trigger()      noexcept       { triggered_ = true; }

    void fill(uint64_t qty) noexcept;
    void cancel() noexcept;
    void print() const;

private:
    uint64_t    id_;
    int64_t     timestamp_us_;
    Side        side_;
    OrderKind   kind_;
    int64_t     price_;          // limit_price for stop-loss orders
    uint64_t    quantity_;
    uint64_t    leaves_qty_;
    OrderStatus status_;

    // Iceberg fields (only used when kind_ == ICEBERG)
    uint64_t display_qty_{0};
    uint64_t hidden_qty_{0};
    uint64_t orig_display_qty_{0};

    // Stop-loss fields (only used when kind_ == STOP_LOSS)
    int64_t  trigger_price_{0};
    bool     triggered_{false};
};
