#pragma once
#include "plugin.h"
#include "../core_services.h"
#include <unordered_map>
#include <shared_mutex>
#include <thread>
#include <atomic>
#include <string>
#include <cstdint>

struct UsageRecord {
    int64_t prompt_tokens = 0;
    int64_t completion_tokens = 0;
    int64_t total_tokens = 0;
    int64_t request_count = 0;
    int64_t period_start = 0;
};

struct QuotaPolicy {
    int64_t max_tokens_per_day = 0;
    int64_t max_tokens_per_month = 0;
    int64_t max_requests_per_day = 0;
    int64_t max_requests_per_month = 0;
};

struct KeyUsageData {
    UsageRecord daily;
    UsageRecord monthly;
    std::unordered_map<std::string, UsageRecord> by_model;
    QuotaPolicy quota;
};

class UsageTrackingPlugin : public Plugin, public core::IAuditSink {
public:
    UsageTrackingPlugin() = default;
    ~UsageTrackingPlugin() override;

    std::string name() const override { return "usage_tracking"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    void shutdown() override;

    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;

    void on_request_completed(const core::AuditEntry& entry) override;

    Json::Value stats() const override;

    [[nodiscard]] Json::Value get_usage_json(const std::string& alias) const;
    [[nodiscard]] Json::Value get_all_usage_json() const;
    bool reset_usage(const std::string& alias);

private:
    void flush();
    void flush_loop();
    void load();
    void save_unlocked() const;
    void rotate_periods_unlocked(KeyUsageData& data) const;

    [[nodiscard]] static int64_t day_start(int64_t epoch);
    [[nodiscard]] static int64_t month_start(int64_t epoch);
    [[nodiscard]] bool check_quota_unlocked(const KeyUsageData& data) const;

    std::unordered_map<std::string, KeyUsageData> usage_;
    mutable std::shared_mutex mtx_;

    std::string persist_path_;
    int flush_interval_seconds_ = 60;
    QuotaPolicy default_quota_;
    bool enabled_ = true;

    std::atomic<bool> running_{false};
    std::jthread flush_thread_;
    std::atomic<bool> dirty_{false};
};
