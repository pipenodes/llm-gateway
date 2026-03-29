#pragma once
#include "../plugin.h"
#include <regex>
#include <vector>
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <cstdint>
#include <atomic>

// RF-32-FUNCIONAL.md — GuardRails LLM Firewall (L1/L2/L3)

struct GuardrailsL1Policy {
    size_t max_payload_bytes = 1024 * 1024;
    std::vector<std::string> allow_models;
    std::vector<std::string> deny_models;
    double requests_per_minute = 60.0;
    double burst = 10.0;
    double block_threshold = 0.7;
    std::vector<std::string> trusted_client_ids;
};

struct GuardrailsL2Policy {
    bool enabled = false;
    double sample_rate = 0.5;
    double toxicity_threshold = 0.8;
    double jailbreak_threshold = 0.7;
};

struct GuardrailsL3Policy {
    bool enabled = false;
    double sample_rate = 0.05;
    std::string judge_model = "llama3:8b";
};

struct GuardrailsTenantPolicy {
    GuardrailsL1Policy l1;
    GuardrailsL2Policy l2;
    GuardrailsL3Policy l3;
};

struct TokenBucket {
    double tokens = 0.0;
    double capacity = 0.0;
    double refill_per_sec = 0.0;
    int64_t last_refill_epoch = 0;
};

struct InjectionPattern {
    std::string name;
    std::regex  pattern;
    float       weight;
};

class GuardrailsPlugin : public Plugin {
public:
    std::string name() const override { return "guardrails"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;
    Json::Value stats() const override;

private:
    const GuardrailsTenantPolicy& get_policy(const std::string& tenant,
                                              const std::string& app) const;
    bool consume_token(const std::string& rate_key, const GuardrailsL1Policy& pol);
    PluginResult check_l1(const Json::Value& body, PluginRequestContext& ctx,
                          const GuardrailsTenantPolicy& policy);
    float score_prompt(const std::string& text) const;
    std::string extract_text(const Json::Value& body) const;

    GuardrailsTenantPolicy default_policy_;
    std::unordered_map<std::string, GuardrailsTenantPolicy> tenant_policies_; // "tenant:app"

    std::vector<InjectionPattern> injection_patterns_;

    mutable std::shared_mutex rate_mtx_;
    std::unordered_map<std::string, TokenBucket> rate_buckets_; // "tenant:client_id"

    mutable std::shared_mutex stats_mtx_;
    std::atomic<int64_t> total_requests_{0};
    std::atomic<int64_t> l1_blocked_{0};
    std::atomic<int64_t> l2_blocked_{0};
};
