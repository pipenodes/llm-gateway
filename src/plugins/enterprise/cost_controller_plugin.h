#pragma once
#include "plugin.h"
#include "../core_services.h"
#include <unordered_map>
#include <shared_mutex>
#include <string>
#include <cstdint>
#include <thread>
#include <atomic>

struct ModelPricing {
    double input_per_1k = 0.0;
    double output_per_1k = 0.0;
};

struct CostBudget {
    double monthly_limit_usd = 0.0;
    double daily_limit_usd = 0.0;
    double spent_monthly_usd = 0.0;
    double spent_daily_usd = 0.0;
    int64_t daily_period_start = 0;
    int64_t monthly_period_start = 0;

    struct ModelCost {
        int64_t requests = 0;
        double cost_usd = 0.0;
    };
    std::unordered_map<std::string, ModelCost> by_model;
};

class CostControllerPlugin : public Plugin, public core::IAuditSink {
public:
    CostControllerPlugin() = default;
    ~CostControllerPlugin() override;

    std::string name() const override { return "cost_controller"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    void shutdown() override;
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;
    Json::Value stats() const override;

    void on_request_completed(const core::AuditEntry& entry) override;

    [[nodiscard]] Json::Value get_all_costs() const;
    [[nodiscard]] Json::Value get_key_costs(const std::string& alias) const;
    void set_budget(const std::string& alias, double monthly, double daily);

private:
    [[nodiscard]] double estimate_cost(const std::string& model, int estimated_tokens) const;
    void record_cost(const std::string& key_alias, const std::string& model,
                     int input_tokens, int output_tokens);
    void rotate_periods(CostBudget& b) const;
    void load();
    void save_unlocked() const;
    void flush_loop();

    std::unordered_map<std::string, ModelPricing> pricing_;
    std::unordered_map<std::string, CostBudget> budgets_;
    mutable std::shared_mutex mtx_;

    double default_monthly_limit_ = 0.0;
    double default_daily_limit_ = 0.0;
    std::string fallback_model_;
    double warning_threshold_ = 0.8;
    bool add_cost_header_ = true;
    std::string persist_path_ = "costs.json";
    int flush_interval_seconds_ = 60;

    std::atomic<bool> running_{false};
    std::atomic<bool> dirty_{false};
    std::jthread flush_thread_;
};
