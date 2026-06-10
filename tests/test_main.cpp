#include "framework.h"
#include <cstdio>

void run_order_tests();
void run_orderbook_tests();
void run_matching_engine_tests();
void run_memory_pool_tests();
void run_concurrent_tests();

int main() {
    std::printf("\n── Order tests ──────────────────────────────\n");
    run_order_tests();

    std::printf("\n── OrderBook tests ──────────────────────────\n");
    run_orderbook_tests();

    std::printf("\n── MatchingEngine tests ─────────────────────\n");
    run_matching_engine_tests();

    std::printf("\n── MemoryPool tests ─────────────────────────\n");
    run_memory_pool_tests();

    std::printf("\n── ConcurrentQueue tests ────────────────────\n");
    run_concurrent_tests();

    std::printf("\n─────────────────────────────────────────────\n");
    return test::summary();
}
