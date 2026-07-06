# Limit Order Book Matching Engine

A high-performance C++17 limit order book matching engine with price-time (FIFO) priority, iceberg orders, stop-loss orders, memory pooling, and a concurrent event-driven architecture.

## Features

- **Price-time priority** — FIFO within each price level using `std::deque`
- **Iceberg orders** — hidden quantity replenished automatically; time priority preserved
- **Stop-loss orders** — triggered by last traded price, processed in batch after each match cycle
- **Custom memory pool** — slab allocator with placement-new and `PoolDeleter` for `shared_ptr` integration
- **Thread-safe order book** — `std::shared_mutex` for concurrent reads, exclusive writes
- **Lock-free concurrent engine** — Michael-Scott MPSC queue decouples producers from the matching thread
- **Market depth visualization** — cumulative volume at each price level with consistent scaling
- **Comprehensive test suite** — 53 tests across all components

## Architecture

```
MatchingEngine          (public API, atomic ID generation, verbose logging)
  └─ OrderBook          (price-time book, matching, cancel/modify, stop-loss)
       ├─ bids_         map<int64_t, deque<OrderPtr>, greater<>>  — best bid first
       ├─ asks_         map<int64_t, deque<OrderPtr>>             — best ask first
       ├─ active_       unordered_map<uint64_t, OrderPtr>         — O(1) lookup
       └─ pending_stops_ / triggered_stops_                       — stop-loss pipeline

MemoryPool<T>           (slab allocator, placement-new outside lock)
ConcurrentMatchingEngine (MPSC event queue + consumer thread)
ConcurrentQueue<T>      (Michael-Scott lock-free queue, cache-line padded)
```

### Price representation

All prices are stored as `int64_t` scaled by `PRICE_SCALE = 10000` (4 decimal places). `$100.00` is stored as `1000000`. No floating point in the hot path.

### Key design decisions

**`cancelLocked()` — deadlock-free modify**  
`modifyOrder()` acquires one `unique_lock`, calls the private `cancelLocked()` (no lock), then `addToBook()`. The original double-lock design caused a deadlock; the private helper eliminates re-entrant locking.

**Stop-loss batch processing**  
`checkStopTriggers()` moves triggered orders to `triggered_stops_`. After each match cycle, `match()` processes them in a `while(!empty()) { batch = move; clear; for each: match }` loop that handles cascading triggers without recursive lock acquisition.

**Iceberg replenishment**  
`replenish()` uses `orig_display_qty_` to size each lot correctly. The order stays at its position in the deque — time priority is preserved across lot boundaries.

**Memory pool outside-lock placement-new**  
`allocate()` grabs a `Block*` under the mutex, then runs placement-new outside it. The block is unreachable to other threads until it's returned to the free list via `deallocate()`, so construction is safe without the lock.

## Building

**Requires:** GCC 8+ / Clang 7+, CMake 3.14+, C++17

```bash
# Default release build (orderbook demo binary)
make

# Debug build with AddressSanitizer + UBSan
make debug

# Run test suite
make tests

# Run benchmarks
make benchmarks
./build-bench/benchmark/lob_benchmark
./build-bench/benchmark/baseline_stl
```

## Benchmark results

Measured on an AMD Ryzen 5 4600H @ 3.0 GHz, WSL2, 200 000-event synthetic workload (65 % limit, 20 % market, 15 % cancel, prices normally distributed ±$1 around $100).

### Order processing latency (single-threaded)

| Metric  | Latency |
|---------|---------|
| Min     | 0.037 µs |
| Avg     | 0.263 µs |
| P50     | 0.181 µs |
| P95     | 0.492 µs |
| P99     | 1.045 µs |
| P99.9   | 2.522 µs |

**Throughput:** ~3.4 M orders/sec (latency mode) · ~4.6 M orders/sec (throughput mode)

### Modify-order latency

| Metric | Latency |
|--------|---------|
| Avg    | 0.255 µs |
| P99    | 0.341 µs |

### Concurrent engine (4 producer threads)

| Metric | Value |
|--------|-------|
| Throughput | ~2.7 M events/sec |
| Events processed | 200 000 |

## Order types

### Limit order
```cpp
MatchingEngine engine;
uint64_t id = engine.submitLimit(Side::BUY, 1000000LL, 100);  // $100.00, qty 100
```

### Market order
```cpp
engine.submitMarket(Side::SELL, 50);   // fills at best available bid
```

### Iceberg order
```cpp
// $100.00, 500 total, 100 visible at a time
uint64_t id = engine.submitIceberg(Side::BUY, 1000000LL, 500, 100);
```

### Stop-loss order
```cpp
// Triggers when last trade ≤ $99.50; then submits limit sell at $99.40
uint64_t id = engine.submitStopLoss(Side::SELL, 995000LL, 994000LL, 100);
```

### Cancel / modify
```cpp
engine.cancelOrder(id);
engine.modifyOrder(id, 985000LL, 80);   // new price + qty, loses time priority
```

## Market depth

```
=== Market Depth (Top 5 Levels) ===
       Price       Qty    Cumulative
------------------------------------
ASKS:
      102.50       100           350
      102.00        85           250
      101.50        70           165
      101.00        55            95
      100.50        40            40
--- spread: $1.00 ---
BIDS:
       99.50        50            50
       99.00        60           110
       98.50        70           180
       98.00        80           260
       97.50        90           350
====================================
```

## Test suite

```bash
make tests
./build-tests/tests/lob_tests
```

53 tests across 5 modules:

| Module | Tests | Covers |
|--------|-------|--------|
| Order | 12 | constructors, fill, replenish, iceberg lots, stop-loss trigger |
| OrderBook | 16 | crossing, FIFO priority, cancel, modify, iceberg, stop-loss |
| MatchingEngine | 11 | submission, thread safety, cancel/modify under concurrency |
| MemoryPool | 8 | alloc/dealloc, growth, address aliasing, concurrent use |
| ConcurrentQueue | 6 | SPSC ordering, MPSC delivery, MPMC stress |

## Project structure

```
include/
  order.h              Order type with iceberg + stop-loss support
  orderbook.h          Price-time book with shared_mutex
  matching_engine.h    Public API + memory pool integration
  memory_pool.h        Slab allocator with PoolDeleter

src/
  order.cpp
  orderbook.cpp
  matching_engine.cpp
  main.cpp             Demo: iceberg, stop-loss, multi-threaded, market depth

benchmark/
  benchmark.h          BenchmarkTimer + PerformanceStats
  order_event.h        OrderEvent struct for queue-based dispatch
  concurrent_queue.h   Michael-Scott MPMC lock-free queue
  lob_benchmark.cpp    Synthetic workload: latency, throughput, modify
  concurrent_matching_engine.h/.cpp  MPSC event-driven engine
  baseline_stl.cpp     Naive STL baseline for comparison

tests/
  framework.h          Minimal test framework (ASSERT, RUN_TEST macros)
  test_order.cpp
  test_orderbook.cpp
  test_matching_engine.cpp
  test_memory_pool.cpp
  test_concurrent.cpp
  test_main.cpp
```

## License

MIT
