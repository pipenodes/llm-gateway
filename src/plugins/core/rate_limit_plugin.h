#pragma once
#include "../plugin.h"
#include "../core_services.h"
#include "../rate_limiter.h"

class RateLimitPlugin : public Plugin, public core::IRateLimiter {
public:
    std::string name() const override { return "rate_limit"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;

    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;

    [[nodiscard]] bool enabled() const noexcept override;
    void setMaxRpm(int rpm) noexcept override;
    [[nodiscard]] core::RateLimitInfo getInfo(const std::string& id) override;
    [[nodiscard]] bool allow(const std::string& id, int custom_rpm = 0) override;

private:
    RateLimiter limiter_;
};
