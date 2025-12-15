cmake_minimum_required(VERSION 3.10)
project(LimitOrderBook VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

include_directories(include)

add_executable(orderbook 
    src/main.cpp
    src/order.cpp
    src/orderbook.cpp
    src/matching_engine.cpp
)

enable_testing()
add_subdirectory(tests)
