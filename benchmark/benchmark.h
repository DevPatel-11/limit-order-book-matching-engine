#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// BenchmarkTimer
//
// High-resolution wall-clock timer. Starts automatically on construction.
// Call stop() when the timed region ends, then query elapsed_*() helpers.
// ---------------------------------------------------------------------------
class BenchmarkTimer {
public:
    BenchmarkTimer() { start(); }

    void start() {
        start_ = std::chrono::high_resolution_clock::now();
    }

    void stop() {
        end_ = std::chrono::high_resolution_clock::now();
    }

    // Nanoseconds between start() and stop().
    double elapsed_ns() const {
        return static_cast<double>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end_ - start_).count());
    }

    // Microseconds between start() and stop().
    double elapsed_us() const { return elapsed_ns() / 1'000.0; }

    // Milliseconds between start() and stop().
    double elapsed_ms() const { return elapsed_ns() / 1'000'000.0; }

private:
    std::chrono::high_resolution_clock::time_point start_;
    std::chrono::high_resolution_clock::time_point end_;
};

// ---------------------------------------------------------------------------
// PerformanceStats
//
// Collects per-operation latency samples (in nanoseconds), computes summary
// statistics, and prints a formatted report in microseconds.
//
// Typical usage:
//   PerformanceStats stats;
//   for (...) {
//       BenchmarkTimer t;
//       do_work();
//       t.stop();
//       stats.add_latency(t.elapsed_ns());
//   }
//   stats.compute();
//   stats.print_summary("my workload");
// ---------------------------------------------------------------------------
class PerformanceStats {
public:
    void add_latency(double ns) {
        latencies_.push_back(ns);
    }

    // Sorts the latency vector and computes all derived statistics.
    // Must be called before print_summary().
    void compute() {
        if (latencies_.empty()) return;

        std::sort(latencies_.begin(), latencies_.end());

        min_ns_   = latencies_.front();
        max_ns_   = latencies_.back();
        sum_ns_   = std::accumulate(latencies_.begin(), latencies_.end(), 0.0);
        avg_ns_   = sum_ns_ / static_cast<double>(latencies_.size());

        p50_ns_   = percentile(50.0);
        p95_ns_   = percentile(95.0);
        p99_ns_   = percentile(99.0);
        p999_ns_  = percentile(99.9);
    }

    // Prints a formatted summary table to stdout. Latencies are reported in
    // microseconds. Call compute() first.
    void print_summary(const std::string& label) const {
        if (latencies_.empty()) {
            std::cout << "[" << label << "] No samples recorded.\n";
            return;
        }

        const int w = 12; // column width
        auto us = [](double ns) { return ns / 1'000.0; };

        std::cout << "\n=== " << label << " ===\n";
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "  Samples : " << latencies_.size() << "\n";
        std::cout << "  Min     : " << std::setw(w) << us(min_ns_)  << " us\n";
        std::cout << "  Avg     : " << std::setw(w) << us(avg_ns_)  << " us\n";
        std::cout << "  P50     : " << std::setw(w) << us(p50_ns_)  << " us\n";
        std::cout << "  P95     : " << std::setw(w) << us(p95_ns_)  << " us\n";
        std::cout << "  P99     : " << std::setw(w) << us(p99_ns_)  << " us\n";
        std::cout << "  P99.9   : " << std::setw(w) << us(p999_ns_) << " us\n";
        std::cout << "  Max     : " << std::setw(w) << us(max_ns_)  << " us\n";
        std::cout << std::endl;
    }

private:
    // Returns the p-th percentile (0–100) using nearest-rank interpolation.
    // Assumes latencies_ is already sorted.
    double percentile(double p) const {
        if (latencies_.size() == 1) return latencies_[0];

        double rank = (p / 100.0) * static_cast<double>(latencies_.size() - 1);
        std::size_t lo = static_cast<std::size_t>(std::floor(rank));
        std::size_t hi = static_cast<std::size_t>(std::ceil(rank));
        double frac    = rank - static_cast<double>(lo);

        return latencies_[lo] + frac * (latencies_[hi] - latencies_[lo]);
    }

    std::vector<double> latencies_; // raw samples, nanoseconds

    // Derived — populated by compute()
    double min_ns_  = 0.0;
    double max_ns_  = 0.0;
    double avg_ns_  = 0.0;
    double sum_ns_  = 0.0;
    double p50_ns_  = 0.0;
    double p95_ns_  = 0.0;
    double p99_ns_  = 0.0;
    double p999_ns_ = 0.0;
};
