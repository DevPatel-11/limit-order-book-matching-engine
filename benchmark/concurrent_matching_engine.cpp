#include "concurrent_matching_engine.h"
#include <chrono>

void ConcurrentMatchingEngine::matcherLoop() {
    OrderEvent evt;
    while (running.load(std::memory_order_acquire)) {
        if (event_queue.try_dequeue(evt)) {
            switch (evt.type) {
                case OrderEventType::SUBMIT_LIMIT:
                case OrderEventType::SUBMIT_MARKET:
                    engine.submitOrder(evt.order_id, evt.side, evt.price, evt.quantity);
                    break;
                case OrderEventType::CANCEL:
                    engine.cancelOrder(evt.order_id);
                    break;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }
}

ConcurrentMatchingEngine::ConcurrentMatchingEngine(size_t max_orders)
    : engine(max_orders) {
    running.store(true, std::memory_order_release);
    matcher_thread = std::thread(&ConcurrentMatchingEngine::matcherLoop, this);
}

ConcurrentMatchingEngine::~ConcurrentMatchingEngine() {
    shutdown();
}

void ConcurrentMatchingEngine::submitEvent(const OrderEvent& evt) {
    event_queue.enqueue(evt);
}

void ConcurrentMatchingEngine::shutdown() {
    if (running.exchange(false, std::memory_order_release)) {
        if (matcher_thread.joinable()) {
            matcher_thread.join();
        }
    }
}
