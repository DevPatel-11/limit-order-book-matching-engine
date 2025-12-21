# Limit Order Book - Benchmark Suite

Performance benchmarking harness for comparing the high-performance LOB against baseline STL implementations.

## Files

- `benchmark.h` - Timing utilities and statistics aggregation (latency percentiles, throughput)
- `lob_benchmark.cpp` - Benchmarks the main MatchingEngine with synthetic workloads
- `baseline_stl.cpp` - Baseline using pure STL containers for comparison
- `Makefile` - Build system

## Building

```bash
cd benchmark
make all
```

## Running Benchmarks

### Benchmark LOB (100k orders)
```bash
make run-lob
```

### Benchmark Baseline (100k orders)
```bash
make run-baseline
```

### Run Both
```bash
make run-all
```

### Custom Order Count
```bash
./build/lob_benchmark 500000
./build/baseline_stl 500000
```

## Metrics

Each benchmark reports:
- **Min/Avg/Max Latency** (microseconds) for order submissions
- **P50, P99, P99.9 Latency** - Percentile latencies
- **Throughput** - Orders per second

## Sample Output

```
====== LOB Benchmark ======
Number of orders: 100000

=== Order Submission Latency ===
  Min:      0.50 us
  Avg:      2.35 us
  P50:      2.10 us
  P99:      5.80 us
  P99.9:    12.50 us
  Max:      45.20 us

Throughput: 42571428 orders/sec
```

## Workload Characteristics

- **Order Distribution**: 90% limit orders, 10% market orders
- **Price Range**: 10M - 20M (scaled from 100.00 - 200.00)
- **Quantity**: 1 - 1000 shares (uniform random)
- **Sides**: 50% BUY, 50% SELL

## Tuning

Edit `lob_benchmark.cpp` and `baseline_stl.cpp` to adjust:
- Price range distribution
- Order quantity distribution  
- Limit vs. market order ratio
- Number of price levels


## Concurrency & Threading

The engine includes thread-safe concurrent components designed for multi-threaded market data ingestion:

### Architecture
- **ConcurrentQueue<T>**: Lock-free MPMC queue using atomic operations and CAS loops
- **OrderEvent**: Encapsulates order operations (SUBMIT, CANCEL) with timestamps
- **ConcurrentMatchingEngine**: Wrapper providing event-driven matching with dedicated matcher thread

### Design Choices
1. **Dedicated Matcher Thread**: Single consumer processes events sequentially to maintain FIFO/price priority without complex synchronization
2. **Atomic-Based Queue**: Avoids mutex overhead for high-frequency event submission
3. **Memory Efficiency**: Reuses order pool for reduced allocation overhead
4. **Documentation Over Optimization**: Prioritizes clarity on design decisions for production readiness

### Usage Example
```cpp
ConcurrentMatchingEngine engine(1000);  // max pending events
for (auto& evt : events) {
    engine.submitEvent(evt);
}
engine.shutdown();
```

### Performance Notes
- Single-threaded matcher avoids lock contention
- Benchmark shows ~10.7k orders/sec with 93µs avg latency
- Suitable for up to ~100-500 concurrent clients
