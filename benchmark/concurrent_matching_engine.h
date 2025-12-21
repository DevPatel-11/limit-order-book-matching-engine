#ifndef CONCURRENT_MATCHING_ENGINE_H
#define CONCURRENT_MATCHING_ENGINE_H

#include "concurrent_queue.h"
#include "order_event.h"
#include "matching_engine.h"
#include <thread>
#include <atomic>
#include <iostream>

class ConcurrentMatchingEngine {
private:
    MatchingEngine engine;
    ConcurrentQueue<OrderEvent> event_queue;
    std::atomic<bool> running{false};
    std::thread matcher_thread;

    void matcherLoop();

public:
    ConcurrentMatchingEngine(size_t max_orders);
    ~ConcurrentMatchingEngine();

    void submitEvent(const OrderEvent& evt);
    void shutdown();
    bool isRunning() const { return running.load(); }
};

#endif
