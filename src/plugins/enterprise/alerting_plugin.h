#pragma once
#include "plugin.h"
#include "../core_services.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <deque>
#include <unordered_map>

class AlertingPlugin : public Plugin, public core::IAuditSink {
public:
    AlertingPlugin() = default;
    ~AlertingPlugin() override;

    std::string name() const override { return "alerting"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    void shutdown() override;

    PluginResult before_request(
        Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(
        Json::Value& response, PluginRequestContext& ctx) override;

    void on_request_completed(const core::AuditEntry& entry) override;

private:
    void worker_loop();
    void try_send_webhook(const std::string& payload);

    std::string webhook_url_;
    int window_seconds_ = 300;
    int status_4xx_limit_ = 10;
    bool alert_on_blocked_ = true;

    std::unordered_map<std::string, std::deque<int64_t>> key_4xx_timestamps_;
    mutable std::mutex mtx_;
    std::queue<std::string> payload_queue_;
    std::condition_variable cv_;
    std::atomic<bool> running_{true};
    std::jthread worker_;
};
