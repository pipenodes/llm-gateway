#pragma once
#include "plugin.h"
#include "../core_services.h"
#include <unordered_map>
#include <shared_mutex>
#include <deque>
#include <chrono>
#include <cstdint>
#include <atomic>

enum class CircuitState { Closed, Open, HalfOpen };

struct BackendHealth {
    CircuitState state = CircuitState::Closed;
    double rate_multiplier = 1.0;
    int consecutive_failures = 0;
    int64_t last_failure_at = 0;
    int64_t circuit_open_until = 0;

    struct Sample {
        int64_t timestamp;
        double latency_ms;
        bool error;
    };
    std::deque<Sample> samples;
};

class AdaptiveRateLimiterPlugin : public Plugin, public core::IAuditSink {
public:
    std::string name() const override { return "adaptive_rate_limiter"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;
    Json::Value stats() const override;

    void on_request_completed(const core::AuditEntry& entry) override;

private:
    void record_sample(const std::string& backend, double latency_ms, bool error);
    void update_health(BackendHealth& h) const;
    void prune_window(BackendHealth& h) const;
    [[nodiscard]] std::string resolve_backend(const PluginRequestContext& ctx) const;
    [[nodiscard]] double compute_p95(const BackendHealth& h) const;
    [[nodiscard]] double compute_error_rate(const BackendHealth& h) const;

    std::unordered_map<std::string, BackendHealth> backends_;
    mutable std::shared_mutex mtx_;

    double latency_warning_ms_ = 5000;
    double latency_critical_ms_ = 15000;
    double error_rate_warning_ = 0.1;
    double error_rate_critical_ = 0.5;
    int circuit_open_seconds_ = 30;
    int circuit_failure_threshold_ = 5;
    double recovery_step_ = 0.25;
    int window_seconds_ = 60;
    int default_rpm_ = 60;
};
