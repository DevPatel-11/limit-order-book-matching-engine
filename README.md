# Limit Order Book Matching Engine

A high-performance, thread-safe limit order book (LOB) matching engine written in modern C++17.

## Features

- Price-time priority (FIFO) matching
- Limit, market, iceberg, and stop-loss orders
- Order cancellation and modification
- Custom slab memory pool for reduced heap pressure
- Thread-safe order book with `std::shared_mutex`
- Lock-free event-driven concurrent matching engine
- Comprehensive test suite

## Build

```bash
make          # release build
make debug    # debug build with ASan/UBSan
make tests    # build and run tests
make benchmarks
```

Or with CMake directly:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/orderbook
```

## Project Structure

```
include/          public headers
src/              implementation + demo main
tests/            unit & integration tests
benchmark/        synthetic workload benchmarks
```

## License

MIT
