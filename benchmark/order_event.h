#pragma once

#include <cstdint>
#include <chrono>

enum class EventKind : uint8_t { SUBMIT_LIMIT, SUBMIT_MARKET, CANCEL };

struct OrderEvent {
    EventKind kind;
    uint64_t  order_id;
    char      side;       // 'B' or 'S'
    int64_t   price;      // scaled integer (price * 10000)
    uint64_t  quantity;
    int64_t   timestamp_us;

    OrderEvent()
        : kind(EventKind::SUBMIT_LIMIT), order_id(0),
          side('B'), price(0), quantity(0), timestamp_us(0) {}

    OrderEvent(EventKind k, uint64_t oid, char s, int64_t p, uint64_t q)
        : kind(k), order_id(oid), side(s), price(p), quantity(q) {
        timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
};
