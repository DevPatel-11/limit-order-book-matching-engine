add_executable(lob_benchmark lob_benchmark.cpp)
target_link_libraries(lob_benchmark PRIVATE lob_core pthread)
target_include_directories(lob_benchmark PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(baseline_stl baseline_stl.cpp)
target_link_libraries(baseline_stl PRIVATE lob_core pthread)
target_include_directories(baseline_stl PRIVATE ${CMAKE_SOURCE_DIR}/include)
