add_executable(lob_tests
  test_main.cpp
  test_order.cpp
  test_orderbook.cpp
  test_matching_engine.cpp
  test_memory_pool.cpp
  test_concurrent.cpp
)

target_link_libraries(lob_tests PRIVATE lob_core pthread)
target_include_directories(lob_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/benchmark
)
