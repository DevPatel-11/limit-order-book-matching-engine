#pragma once

#include <chrono>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>

class BenchmarkTimer {
public:
    using clock_t = std::chrono::high_resolution_clock;
    using ns_t = std::chrono::nanoseconds;
    
    BenchmarkTimer() { start(); }
    
    void start() {
        start_time = clock_t::now();
    }
    
    void stop() {
        end_time = clock_t::now();
    }
    
    double elapsed_ns() const {
        return std::chrono::duration_cast<ns_t>(end_time - start_time).count();
    }
    
    double elapsed_us() const {
        return elapsed_ns() / 1000.0;
    }
    
    double elapsed_ms() const {
        return elapsed_ns() / 1000000.0;
    }
    
private:
    clock_t::time_point start_time;
    clock_t::time_point end_time;
};

class PerformanceStats {
public:
    void add_latency(double latency_ns) {
        latencies.push_back(latency_ns);
    }
    
    void compute() {
        if (latencies.empty()) return;
        
        std::sort(latencies.begin(), latencies.end());
        
        double sum = 0;
        for (auto l : latencies) sum += l;
        avg = sum / latencies.size();
        
        min_val = latencies.front();
        max_val = latencies.back();
        
        size_t p50_idx = latencies.size() / 2;
        size_t p99_idx = (latencies.size() * 99) / 100;
        size_t p999_idx = (latencies.size() * 999) / 1000;
        
        p50 = latencies[p50_idx];
        p99 = latencies[std::min(p99_idx, latencies.size() - 1)];
        p999 = latencies[std::min(p999_idx, latencies.size() - 1)];
    }
    
    void print_summary(const std::string& name) const {
        std::cout << "\n=== " << name << " ==="  << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  Min:      " << min_val / 1000.0 << " us" << std::endl;
        std::cout << "  Avg:      " << avg / 1000.0 << " us" << std::endl;
        std::cout << "  P50:      " << p50 / 1000.0 << " us" << std::endl;
        std::cout << "  P99:      " << p99 / 1000.0 << " us" << std::endl;
        std::cout << "  P99.9:    " << p999 / 1000.0 << " us" << std::endl;
        std::cout << "  Max:      " << max_val / 1000.0 << " us" << std::endl;
    }
    
public:
    std::vector<double> latencies;
    double min_val = 0, avg = 0, max_val = 0;
    double p50 = 0, p99 = 0, p999 = 0;
};
