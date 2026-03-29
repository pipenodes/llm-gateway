#pragma once
#include "../plugin.h"
#include "../core_services.h"
#include <unordered_map>
#include <shared_mutex>
#include <deque>
#include <string>
#include <vector>
#include <random>
#include <cstdint>
#include <optional>

struct ABVariant {
    std::string model;
    std::string provider;
    int weight = 0;
};

struct ABVariantMetrics {
    std::string model;
    int64_t request_count = 0;
    int64_t total_tokens = 0;
    int64_t error_count = 0;
    std::deque<double> latencies;
    double total_latency_ms = 0;
};

struct ABExperiment {
    std::string name;
    std::string trigger_model;
    std::vector<ABVariant> variants;
    bool enabled = true;
    int64_t created_at = 0;

    std::unordered_map<std::string, ABVariantMetrics> metrics;
};

class ABTestingPlugin : public Plugin, public core::IAuditSink {
public:
    ABTestingPlugin() = default;

    std::string name() const override { return "ab_testing"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;
    Json::Value stats() const override;

    void on_request_completed(const core::AuditEntry& entry) override;

    [[nodiscard]] std::optional<std::string> resolve(const std::string& requested_model,
                                                      const std::string& request_id);
    void record(const std::string& experiment_name, const std::string& variant_model,
                int tokens, double latency_ms, bool error);
    bool add_experiment(const ABExperiment& exp);
    bool remove_experiment(const std::string& exp_name);
    [[nodiscard]] Json::Value get_results(const std::string& exp_name) const;
    [[nodiscard]] Json::Value list_experiments() const;

private:
    [[nodiscard]] double compute_percentile(const std::deque<double>& vals, double pct) const;

    std::unordered_map<std::string, ABExperiment> experiments_;
    std::unordered_map<std::string, std::string> request_to_experiment_;
    std::unordered_map<std::string, std::string> request_to_variant_;
    mutable std::shared_mutex mtx_;

    thread_local static std::mt19937 rng_;
    static constexpr size_t MAX_LATENCY_SAMPLES = 1000;
};
