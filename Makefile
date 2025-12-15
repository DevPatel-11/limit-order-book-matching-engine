.PHONY: all release debug tests benchmarks clean run

BUILD_DIR ?= build

all: release

release:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_DIR) --parallel

debug:
	cmake -S . -B $(BUILD_DIR)-debug -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DIR)-debug --parallel

tests:
	cmake -S . -B $(BUILD_DIR)-tests -DCMAKE_BUILD_TYPE=Debug \
	      -DBUILD_TESTS=ON
	cmake --build $(BUILD_DIR)-tests --parallel
	./$(BUILD_DIR)-tests/tests/lob_tests

benchmarks:
	cmake -S . -B $(BUILD_DIR)-bench -DCMAKE_BUILD_TYPE=Release \
	      -DBUILD_BENCHMARKS=ON
	cmake --build $(BUILD_DIR)-bench --parallel

run: release
	./$(BUILD_DIR)/orderbook

clean:
	rm -rf $(BUILD_DIR) $(BUILD_DIR)-debug $(BUILD_DIR)-tests $(BUILD_DIR)-bench
