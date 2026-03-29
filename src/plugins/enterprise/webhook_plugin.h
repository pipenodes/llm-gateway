#pragma once
#include "../plugin.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

enum class WebhookEventType {
    BackendDown, BackendUp, RateLimitHit, QuotaWarning, QuotaExceeded,
    KeyCreated, KeyRevoked, AuthFailure, HighLatency, ErrorSpike, Custom
};

struct WebhookEvent {
    WebhookEventType type;
    std::string type_name;
    int64_t timestamp = 0;
    Json::Value data;
};

struct WebhookEndpoint {
    std::string name;
    std::string url;
    std::unordered_set<std::string> event_types;
    std::unordered_map<std::string, std::string> headers;
    int max_retries = 3;
    int timeout_ms = 5000;
    bool enabled = true;

    int64_t events_sent = 0;
    int64_t events_failed = 0;
};

class WebhookPlugin : public Plugin {
public:
    WebhookPlugin() = default;
    ~WebhookPlugin() override;

    std::string name() const override { return "webhook"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    void shutdown() override;
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;
    Json::Value stats() const override;

    void emit(const std::string& event_type, const Json::Value& data);
    void emit(WebhookEventType type, const Json::Value& data);

    [[nodiscard]] Json::Value list_endpoints() const;
    bool send_test_event();

private:
    void worker_loop();
    bool send_to_endpoint(WebhookEndpoint& ep, const WebhookEvent& event);
    [[nodiscard]] static std::string mask_url(const std::string& url);
    [[nodiscard]] static std::string event_type_to_string(WebhookEventType type);

    std::vector<WebhookEndpoint> endpoints_;
    std::queue<WebhookEvent> event_queue_;
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> running_{false};
    std::jthread worker_;
    size_t max_queue_size_ = 10000;
};
