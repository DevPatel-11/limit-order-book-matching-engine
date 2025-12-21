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

