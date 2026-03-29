#include "request_deduplication_plugin.h"
#include <json/json.h>
#include <sstream>
#include <functional>

bool RequestDeduplicationPlugin::init(const Json::Value& config) {
    window_seconds_ = config.get("window_seconds", 60).asInt();
    max_entries_ = config.get("max_entries", 1000).asUInt();
    return true;
}

std::string RequestDeduplicationPlugin::hash_request(const Json::Value& body) const {
    Json::StreamWriterBuilder w;
    w["indentation"] = "";
    std::string canonical = Json::writeString(w, body);
    return std::to_string(std::hash<std::string>{}(canonical));
}

void RequestDeduplicationPlugin::cleanup_expired() {
    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - std::chrono::seconds(window_seconds_);
    for (auto it = cache_.begin(); it != cache_.end(); ) {
        if (it->second.second < cutoff) it = cache_.erase(it);
        else ++it;
    }
    while (cache_.size() > max_entries_ && !cache_.empty())
        cache_.erase(cache_.begin());
}

PluginResult RequestDeduplicationPlugin::before_request(Json::Value& body, PluginRequestContext& ctx) {
    std::string key = hash_request(body);
    std::lock_guard lock(mtx_);
    cleanup_expired();
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        PluginResult r;
        r.action = PluginAction::Continue;
        r.cached_response = it->second.first;
        ctx.metadata["request_dedup_hit"] = "1";
        return r;
    }
    ctx.metadata["request_dedup_key"] = key;
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult RequestDeduplicationPlugin::after_response(Json::Value& response, PluginRequestContext& ctx) {
    auto it = ctx.metadata.find("request_dedup_key");
    if (it == ctx.metadata.end()) return PluginResult{.action = PluginAction::Continue};
    Json::StreamWriterBuilder w;
    w["indentation"] = "";
    std::string response_str = Json::writeString(w, response);
    std::lock_guard lock(mtx_);
    cache_[it->second] = {response_str, std::chrono::steady_clock::now()};
    return PluginResult{.action = PluginAction::Continue};
}
