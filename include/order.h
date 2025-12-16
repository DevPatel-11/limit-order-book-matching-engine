#pragma once

#include <cstdint>

constexpr int64_t PRICE_SCALE = 10000;

enum class Side        : uint8_t { BUY, SELL };
enum class OrderKind   : uint8_t { LIMIT, MARKET };
enum class OrderStatus : uint8_t { ACTIVE, PARTIAL, FILLED, CANCELLED };

class Order {
public:
    Order(uint64_t id, int64_t timestamp_us, Side side, OrderKind kind,
          int64_t price, uint64_t qty);

    uint64_t    id()        const noexcept { return id_; }
    int64_t     timestamp() const noexcept { return timestamp_us_; }
    Side        side()      const noexcept { return side_; }
    OrderKind   kind()      const noexcept { return kind_; }
    int64_t     price()     const noexcept { return price_; }
    uint64_t    quantity()  const noexcept { return quantity_; }
    uint64_t    leaves()    const noexcept { return leaves_qty_; }
    OrderStatus status()    const noexcept { return status_; }

    bool isFilled() const noexcept { return leaves_qty_ == 0; }
    bool isMarket() const noexcept { return kind_ == OrderKind::MARKET; }
    bool isActive() const noexcept {
        return status_ == OrderStatus::ACTIVE || status_ == OrderStatus::PARTIAL;
    }

    void fill(uint64_t qty) noexcept;
    void cancel() noexcept;
    void print() const;

private:
    uint64_t    id_;
    int64_t     timestamp_us_;
    Side        side_;
    OrderKind   kind_;
    int64_t     price_;
    uint64_t    quantity_;
    uint64_t    leaves_qty_;
    OrderStatus status_;
};
