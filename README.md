# Limit Order Book & Matching Engine

A high-performance exchange-style limit order book implementation with price-time priority matching, similar to what real financial markets use.

## Features

- **Price-Time Priority Matching**: Orders matched at best price, with time priority for same price levels
- **Order Types**: Support for Limit Buy/Sell and Market Buy/Sell orders
- **Partial Fills**: Orders can be partially filled across multiple trades
- **Real-time Book State**: View bid/ask depth and spread
- **Trade History**: Complete record of all executed trades

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
    
    // Add limit orders
    engine.submitLimitOrder(OrderSide::BUY, 100.00, 50);
    engine.submitLimitOrder(OrderSide::SELL, 101.00, 60);
    
    // Submit market order
    engine.submitMarketOrder(OrderSide::BUY, 30);
    
    // View order book
    engine.printBook();
    engine.printTrades();
    engine.printStats();
    
    return 0;
}
```

## Sample Output

```
========== ORDER BOOK ==========
ASKS (Sell Orders):
  102.00 | 100
  101.50 | 50
  101.00 | 30
--------------------------------
Spread: 2.00
--------------------------------
BIDS (Buy Orders):
  99.00 | 75
  99.50 | 25
================================

========== TRADE HISTORY ==========
Trade: Buy#7 <-> Sell#4 | 60@101.00
Trade: Buy#7 <-> Sell#5 | 30@101.50
Trade: Buy#10 <-> Sell#5 | 50@101.50
Trade: Buy#11 <-> Sell#8 | 120@99.75
===================================
```

## Performance Characteristics

- **Order Submission**: O(log N) where N is the number of price levels
- **Matching**: O(M) where M is the number of orders matched
- **Book Lookup**: O(1) for best bid/ask
- **Space**: O(N) for number of active orders

## Future Enhancements

- [ ] Order cancellation by ID
- [ ] Order modification
- [ ] Iceberg orders (hidden quantity)
- [ ] Stop-loss orders
- [ ] Market depth visualization
- [ ] Performance benchmarking
- [ ] Multi-threaded matching
- [ ] Memory pool optimization

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
