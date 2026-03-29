#ifndef BENCHMARK_LATENCY_METER_H
#define BENCHMARK_LATENCY_METER_H

#include <chrono>

namespace benchmark {

struct LatencyMeter {
    using Clock = std::chrono::steady_clock;
    Clock::time_point start_;
    Clock::time_point first_token_;
    Clock::time_point end_;
    bool first_token_recorded_ = false;

    void start() {
        start_ = Clock::now();
        first_token_recorded_ = false;
    }
    void record_first_token() {
        if (!first_token_recorded_) {
            first_token_ = Clock::now();
            first_token_recorded_ = true;
        }
    }
    void stop() { end_ = Clock::now(); }

    double ttft_ms() const {
        if (!first_token_recorded_) return -1.0;
        return std::chrono::duration<double, std::milli>(first_token_ - start_).count();
    }
    double total_ms() const {
        return std::chrono::duration<double, std::milli>(end_ - start_).count();
    }
};

} // namespace benchmark

#endif
