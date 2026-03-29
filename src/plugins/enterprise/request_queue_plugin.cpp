#include "request_queue_plugin.h"

#if !EDGE_CORE

bool RequestQueuePlugin::init(const Json::Value& config) {
    enabled_ = config.get("enabled", true).asBool();
    size_t max_size = config.get("max_size", 1000).asUInt();
    size_t max_concurrency = config.get("max_concurrency", 4).asUInt();
    size_t worker_threads = config.get("worker_threads", 4).asUInt();
    queue_ = std::make_unique<RequestQueue>(max_size, max_concurrency, worker_threads);
    queue_->set_enabled(enabled_);
    return true;
}

void RequestQueuePlugin::shutdown() {
    if (queue_) queue_->shutdown();
}

PluginResult RequestQueuePlugin::before_request(Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult RequestQueuePlugin::after_response(Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}

bool RequestQueuePlugin::enabled() const noexcept {
    return enabled_ && queue_ && queue_->enabled();
}

core::EnqueueResult RequestQueuePlugin::enqueue(core::Priority priority,
    std::chrono::milliseconds timeout,
    std::function<void()> task,
    const std::string& request_id,
    const std::string& key_alias) {
    if (!queue_) {
        std::promise<bool> p;
        p.set_value(false);
        return {false, 0, p.get_future()};
    }
    auto r = queue_->enqueue(static_cast<Priority>(priority), timeout, std::move(task), request_id, key_alias);
    return {r.accepted, r.position, std::move(r.completion)};
}

size_t RequestQueuePlugin::pending() const noexcept {
    return queue_ ? queue_->pending() : 0;
}

Json::Value RequestQueuePlugin::stats() const {
    return queue_ ? queue_->stats() : Json::Value(Json::objectValue);
}

void RequestQueuePlugin::clear_pending() {
    if (queue_) queue_->clear_pending();
}

#endif // !EDGE_CORE
