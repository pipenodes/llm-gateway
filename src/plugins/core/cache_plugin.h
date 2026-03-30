#pragma once
#include "plugin.h"
#include "core_services.h"
#include "cache.h"
#include <memory>

class CachePlugin : public Plugin, public core::ICache {
public:
    std::string name() const override { return "cache"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;

    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;

    [[nodiscard]] bool enabled() const noexcept override;
    [[nodiscard]] std::optional<std::string> get(const core::CacheKey& key) override;
    void put(const core::CacheKey& key, const std::string& response) override;
    [[nodiscard]] Json::Value stats() const override;
    [[nodiscard]] core::CacheStats cacheStats() const override;

private:
    ResponseCache cache_;
};
