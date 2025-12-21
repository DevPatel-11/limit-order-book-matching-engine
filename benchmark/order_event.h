#ifndef ORDER_EVENT_H
#define ORDER_EVENT_H

#include <cstdint>
#include <chrono>

enum class OrderEventType {
    SUBMIT_LIMIT,
    SUBMIT_MARKET,
    CANCEL
};

struct OrderEvent {
    OrderEventType type;
    uint64_t order_id;
    char side;  // 'B' for buy, 'S' for sell
    uint64_t price;  // integer representation
    uint64_t quantity;
    int64_t timestamp_us;  // microseconds since epoch

    OrderEvent() : type(OrderEventType::SUBMIT_LIMIT), order_id(0),
                   side('B'), price(0), quantity(0), timestamp_us(0) {}

    OrderEvent(OrderEventType t, uint64_t oid, char s, uint64_t p, uint64_t q)
        : type(t), order_id(oid), side(s), price(p), quantity(q) {
        auto now = std::chrono::high_resolution_clock::now();
        timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();
    }
};

#endif
