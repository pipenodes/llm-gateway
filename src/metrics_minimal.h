#pragma once
#include "metrics_interface.h"
#include <atomic>
#include <chrono>

struct MinimalMetricsCollector : IMetricsCollector {
    std::atomic<uint64_t> requests_total{0};
    std::atomic<int32_t> requests_active{0};
    std::chrono::steady_clock::time_point start_time;

    MinimalMetricsCollector() noexcept : start_time(std::chrono::steady_clock::now()) {}

    void begin_request() override;
    void end_request() override;

    [[nodiscard]] std::string toJson(const core::ICache* cache,
        const Json::Value& plugin_metrics = Json::Value()) const override;
    [[nodiscard]] std::string toPrometheus(const core::ICache* cache,
        const Json::Value& plugin_metrics = Json::Value()) const override;
};
