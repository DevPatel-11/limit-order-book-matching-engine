cmake_minimum_required(VERSION 3.14)
project(LimitOrderBook VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -fsanitize=address,undefined")

include_directories(include)

# Core library — shared by main, tests, and benchmarks
add_library(lob_core STATIC
  src/order.cpp
  src/orderbook.cpp
  src/matching_engine.cpp
)
target_include_directories(lob_core PUBLIC include)

find_package(Threads REQUIRED)
target_link_libraries(lob_core PUBLIC Threads::Threads)

add_executable(orderbook src/main.cpp)
target_link_libraries(orderbook PRIVATE lob_core)

option(BUILD_TESTS      "Build the test suite"  OFF)
option(BUILD_BENCHMARKS "Build benchmarks"      OFF)

if(BUILD_TESTS)
  add_subdirectory(tests)
endif()

if(BUILD_BENCHMARKS)
  add_subdirectory(benchmark)
endif()
