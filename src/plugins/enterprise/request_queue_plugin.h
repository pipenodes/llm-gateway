#pragma once
#include "../plugin.h"
#include "../core_services.h"
#if !EDGE_CORE
#include "../request_queue.h"
#endif

class RequestQueuePlugin : public Plugin, public core::IRequestQueue {
public:
    std::string name() const override { return "request_queue"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    void shutdown() override;

    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;

    [[nodiscard]] bool enabled() const noexcept override;
    [[nodiscard]] core::EnqueueResult enqueue(core::Priority priority,
        std::chrono::milliseconds timeout,
        std::function<void()> task,
        const std::string& request_id = "",
        const std::string& key_alias = "") override;
    [[nodiscard]] size_t pending() const noexcept override;
    [[nodiscard]] Json::Value stats() const override;
    void clear_pending() override;

private:
#if !EDGE_CORE
    std::unique_ptr<RequestQueue> queue_;
#endif
    bool enabled_ = false;
};
