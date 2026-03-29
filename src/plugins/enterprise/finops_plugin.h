#pragma once
#include "../plugin.h"
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <atomic>
#include <thread>
#include <cstdint>

// RF-35-FUNCIONAL.md — FinOps: governança de custos multi-tenant

struct ModelPricing {
    double input_per_1k  = 0.0;
    double output_per_1k = 0.0;
};

struct FinOpsBudget {
    double monthly_limit_usd = 0.0;
    double daily_limit_usd   = 0.0;
    double spent_monthly_usd = 0.0;
    double spent_daily_usd   = 0.0;
    int64_t daily_period_start   = 0;
    int64_t monthly_period_start = 0;
    // Alert thresholds already fired in current period
    bool alert_50_sent  = false;
    bool alert_80_sent  = false;
    bool alert_100_sent = false;
};

// Hierarchical cost node: aggregates cost for one dimension (tenant/app/client)
struct CostNode {
    int64_t requests   = 0;
    int64_t input_tokens  = 0;
    int64_t output_tokens = 0;
    double  cost_usd   = 0.0;
    std::unordered_map<std::string, double> by_model; // model → cost
};

class FinOpsPlugin : public Plugin {
public:
    std::string name() const override { return "finops"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;
    void shutdown() override;
    Json::Value stats() const override;

    // Admin API helpers
    Json::Value get_costs(const std::string& tenant_id) const;
    Json::Value get_budget(const std::string& tenant_id) const;
    void set_tenant_budget(const std::string& tenant_id, double monthly, double daily);
    std::string export_csv() const; // returns CSV content for all tenants

private:
    double compute_cost(const std::string& model, int input_tokens, int output_tokens) const;
    void record_usage(const std::string& tenant, const std::string& app,
                      const std::string& client, const std::string& model,
                      int input_tokens, int output_tokens);
    void rotate_period(FinOpsBudget& b) const;
    void check_and_alert(const std::string& tenant, FinOpsBudget& b);
    void send_webhook_async(const std::string& url, const Json::Value& payload) const;
    bool is_over_budget(const std::string& tenant, const std::string& app,
                        const std::string& client, double est_cost) const;
    void load();
    void save_unlocked() const;
    void flush_loop();

    std::unordered_map<std::string, ModelPricing> pricing_;

    mutable std::shared_mutex mtx_;
    // Hierarchical cost nodes: key = "tenant", "tenant:app", "tenant:app:client"
    std::unordered_map<std::string, CostNode> cost_nodes_;
    // Budgets: keyed by tenant, "tenant:app", or "tenant:app:client"
    std::unordered_map<std::string, FinOpsBudget> budgets_;

    std::string persist_path_;
    int flush_interval_seconds_ = 60;
    std::string alert_webhook_url_;

    std::atomic<bool> running_{false};
    std::jthread flush_thread_;
    std::atomic<bool> dirty_{false};

    std::atomic<int64_t> total_requests_{0};
    std::atomic<int64_t> budget_blocks_{0};
};
