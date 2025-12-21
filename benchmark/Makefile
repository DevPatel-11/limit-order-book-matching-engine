CXX = g++
CXXFLAGS = -std=c++17 -O3 -march=native -Wall -Wextra
INCLUDES = -I../include
LIBS = -lpthread

BUILD_DIR = build

LOB_BENCH = $(BUILD_DIR)/lob_benchmark
BASELINE = $(BUILD_DIR)/baseline_stl

all: $(LOB_BENCH) $(BASELINE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LOB_BENCH): lob_benchmark.cpp ../src/order.cpp ../src/orderbook.cpp ../src/matching_engine.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

$(BASELINE): baseline_stl.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

run-lob: $(LOB_BENCH)
	$(LOB_BENCH) 100000

run-baseline: $(BASELINE)
	$(BASELINE) 100000

run-all: run-lob run-baseline

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run-lob run-baseline run-all clean
