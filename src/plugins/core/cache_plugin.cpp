#include "cache_plugin.h"

bool CachePlugin::init(const Json::Value& config) {
    cache_.cache_enabled = config.get("enabled", true).asBool();
    cache_.ttl_seconds = config.get("ttl_seconds", 300).asInt();
    cache_.max_entries = config.get("max_entries", 1000).asUInt();
    int max_mb = config.get("max_memory_mb", 0).asInt();
    if (max_mb > 0)
        cache_.max_memory_bytes = static_cast<size_t>(max_mb) * 1024 * 1024;
    return true;
}

PluginResult CachePlugin::before_request(Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult CachePlugin::after_response(Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}

bool CachePlugin::enabled() const noexcept {
    return cache_.enabled();
}

std::optional<std::string> CachePlugin::get(const core::CacheKey& key) {
    ResponseCache::CacheKey k;
    k.hash = key.hash;
    k.full = key.full;
    return cache_.get(k);
}

void CachePlugin::put(const core::CacheKey& key, const std::string& response) {
    ResponseCache::CacheKey k;
    k.hash = key.hash;
    k.full = key.full;
    cache_.put(k, response);
}

Json::Value CachePlugin::stats() const {
    auto s = cache_.stats();
    Json::Value out(Json::objectValue);
    out["hits"] = static_cast<Json::UInt64>(s.hits);
    out["misses"] = static_cast<Json::UInt64>(s.misses);
    out["evictions"] = static_cast<Json::UInt64>(s.evictions);
    out["entries"] = static_cast<Json::UInt64>(s.entries);
    out["memory_bytes"] = static_cast<Json::UInt64>(s.memory_bytes);
    out["hit_rate"] = s.hit_rate;
    return out;
}

core::CacheStats CachePlugin::cacheStats() const {
    auto s = cache_.stats();
    return {s.hits, s.misses, s.evictions, s.entries, s.memory_bytes, s.hit_rate};
}
