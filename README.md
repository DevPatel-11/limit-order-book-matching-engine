# Limit Order Book & Matching Engine

A high-performance exchange-style limit order book implementation with price-time priority matching, similar to what real financial markets use.

## Features

- **Price-Time Priority Matching**: Orders matched at best price, with time priority for same price levels
- **Order Types**: Support for Limit Buy/Sell and Market Buy/Sell orders
- **Partial Fills**: Orders can be partially filled across multiple trades
- **Order Management**: Cancel and modify existing orders by ID
- **Real-time Book State**: View bid/ask depth and spread
- **Trade History**: Complete record of all executed trades
- **Active Order Tracking**: Monitor all live orders in the book

## Project Structure

```
limit-order-book-matching-engine/
├── include/
│   ├── order.h              # Order class definition
│   ├── orderbook.h          # OrderBook with bid/ask management
│   └── matching_engine.h    # Matching engine interface
├── src/
│   ├── order.cpp            # Order implementation
│   ├── orderbook.cpp        # OrderBook matching logic
│   ├── matching_engine.cpp  # Engine implementation
│   └── main.cpp             # Demo application
├── CMakeLists.txt
├── Makefile
└── README.md
```

## Core Concepts

### Market Microstructure

The order book maintains two sides:
- **Bids** (Buy orders): Sorted in descending price order
- **Asks** (Sell orders): Sorted in ascending price order

### Matching Rules

1. Buy orders match against the lowest ask price
2. Sell orders match against the highest bid price
3. At the same price level, older orders have priority (FIFO)
4. Partial fills are allowed when quantity doesn't match exactly
5. Unfilled limit orders remain in the book

### Data Structures

- **Bids**: `std::map<double, std::queue<OrderPtr>, std::greater<double>>`
- **Asks**: `std::map<double, std::queue<OrderPtr>, std::less<double>>`
- **Active Orders**: `std::unordered_map<uint64_t, OrderPtr>` for O(1) lookup
- Price levels maintain FIFO queues for time priority

## Building

### Using Make

```bash
make           # Build the project
make run       # Build and run
make clean     # Clean build artifacts
```

### Using CMake

```bash
mkdir build && cd build
cmake ..
make
./orderbook
```

## Usage Example

```cpp
#include "matching_engine.h"

int main() {
    MatchingEngine engine;
    
    // Submit limit orders (returns order ID)
    uint64_t order1 = engine.submitLimitOrder(OrderSide::BUY, 100.00, 50);
    uint64_t order2 = engine.submitLimitOrder(OrderSide::SELL, 101.00, 60);
    
    // Submit market order
    engine.submitMarketOrder(OrderSide::BUY, 30);
    
    // Cancel an order
    engine.cancelOrder(order1);
    
    // Modify an order (change price and/or quantity)
    engine.modifyOrder(order2, 100.50, 75);
    
    // View order book
    engine.printBook();
    engine.printTrades();
    engine.printStats();
    
    return 0;
}
```

## New Features

### Order Cancellation

Cancel any active order by its ID:

```cpp
uint64_t order_id = engine.submitLimitOrder(OrderSide::BUY, 99.50, 100);
// ... later
bool success = engine.cancelOrder(order_id);
```

- Returns `true` if order was successfully canceled
- Returns `false` if order not found or already filled
- Removes order from book and active orders tracking

### Order Modification

Modify price and/or quantity of an existing order:

```cpp
uint64_t order_id = engine.submitLimitOrder(OrderSide::SELL, 101.00, 50);
// ... later
bool success = engine.modifyOrder(order_id, 100.75, 75);
```

- Cancels the original order and replaces it with new parameters
- Maintains the same order ID and timestamp (preserves time priority)
- Returns `true` if successful, `false` if order not found

## Sample Output

```
========== ORDER BOOK ==========
ASKS (Sell Orders):
  102.00 | 100
  101.00 | 60
--------------------------------
Spread: 1.00
Active Orders: 5
--------------------------------
BIDS (Buy Orders):
  100.50 | 75
  99.00 | 75
================================

[CANCEL] Attempting to cancel order #2
[SUCCESS] Order #2 has been canceled

[MODIFY] Attempting to modify order #4 to 50@100.75
[SUCCESS] Order #4 has been modified

========== TRADE HISTORY ==========
Trade: Buy#7 <-> Sell#4 | 40@100.75
===================================
```

## Performance Characteristics

- **Order Submission**: O(log N) where N is the number of price levels
- **Order Cancellation**: O(M) where M is orders at that price level
- **Order Modification**: O(M + log N) (cancel + reinsert)
- **Matching**: O(M) where M is the number of orders matched
- **Book Lookup**: O(1) for best bid/ask
- **Order Lookup**: O(1) via unordered_map
- **Space**: O(N) for number of active orders

## API Reference

### MatchingEngine Methods

| Method | Description | Returns |
|--------|-------------|----------|
| `submitLimitOrder(side, price, qty)` | Submit a limit order | `uint64_t` order ID |
| `submitMarketOrder(side, qty)` | Submit a market order | `void` |
| `cancelOrder(order_id)` | Cancel an active order | `bool` success |
| `modifyOrder(order_id, price, qty)` | Modify existing order | `bool` success |
| `printBook()` | Display current order book | `void` |
| `printTrades()` | Show trade history | `void` |
| `printStats()` | Display statistics | `void` |

## Future Enhancements


- [x] Order cancellation by ID
- [x] Order modification
- [x] Iceberg orders (hidden quantity)
- [x] Stop-loss orders
- [ ] Market depth visualization
- [ ] Performance benchmarking
- [x] Multi-threaded matching
- [x] Memory pool optimization

## Technical Details

### Order Attributes

- `order_id`: Unique identifier
- `timestamp`: Microsecond precision for time priority
- `price`: Limit price (0 for market orders)
- `quantity`: Total quantity
- `remaining_quantity`: Unfilled quantity
- `type`: LIMIT_BUY, LIMIT_SELL, MARKET_BUY, MARKET_SELL
- `side`: BUY or SELL

### Trade Structure

- `buy_order_id`: ID of buy order
- `sell_order_id`: ID of sell order
- `price`: Execution price
- `quantity`: Traded quantity
- `timestamp`: Trade execution time

## Requirements

- C++17 or higher
- CMake 3.10+ (for CMake build)
- g++ or clang++ with C++17 support

## License

MIT License

## Author

Built for learning market microstructure and exchange systems.

### Memory Pool Optimization

The engine uses a custom memory pool for Order objects to minimize heap allocations:

```cpp
// Memory pool is automatically initialized
MatchingEngine engine;  // Creates pool with 1000 pre-allocated slots

// Orders are allocated from the pool
uint64_t order_id = engine.submitLimitOrder(OrderSide::BUY, 100.00, 50);

// View memory pool statistics
engine.printPoolStats();
```

**Benefits:**
- Reduces memory fragmentation
- Faster allocation/deallocation (no system calls)
- Thread-safe with mutex protection
- Automatic growth when pool is exhausted
- Custom deleter integrates with shared_ptr

**Performance:**
- Initial capacity: 1000 orders
- Automatic expansion in blocks of 1000
- O(1) allocation from free list
- O(1) deallocation back to pool


### Multi-Threaded Matching

The engine is fully thread-safe and supports concurrent order submissions:

```cpp
#include <thread>

// Multiple threads can safely submit orders concurrently
std::thread t1([&]() {
    engine.submitLimitOrder(OrderSide::BUY, 100.00, 50);
});

std::thread t2([&]() {
    engine.submitLimitOrder(OrderSide::SELL, 101.00, 60);
});

t1.join();
t2.join();
```

**Thread Safety:**
- `std::shared_mutex` for reader-writer locks
- Exclusive locks (`unique_lock`) for writes (matchOrder, cancelOrder, modifyOrder)
- Shared locks (`shared_lock`) for reads (getBestBid, printBook, printStats)
- Memory pool has internal mutex protection
- Atomic order ID generation

**Performance:**
- Multiple threads can read order book state concurrently
- Write operations are serialized for correctness
- Lock-free order ID generation using atomics
- Demonstrates horizontal scalability for high-throughput scenarios

