#ifndef BENCHMARK_TOKEN_COUNTER_H
#define BENCHMARK_TOKEN_COUNTER_H

namespace benchmark {

struct TokenCounter {
    int input_tokens = 0;
    int output_tokens = 0;

    double tokens_per_sec(double total_latency_ms) const {
        if (total_latency_ms <= 0) return 0.0;
        return (output_tokens * 1000.0) / total_latency_ms;
    }
};

} // namespace benchmark

#endif
