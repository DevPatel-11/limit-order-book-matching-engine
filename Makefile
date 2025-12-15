CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude

SRCS = src/main.cpp src/order.cpp src/orderbook.cpp src/matching_engine.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = orderbook

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	 rm -rf build

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
