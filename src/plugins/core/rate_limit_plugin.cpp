#include "rate_limit_plugin.h"

bool RateLimitPlugin::init(const Json::Value& config) {
    int rpm = config.get("max_rpm", 0).asInt();
    limiter_.setMaxRpm(rpm);
    return true;
}

PluginResult RateLimitPlugin::before_request(Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult RateLimitPlugin::after_response(Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}

bool RateLimitPlugin::enabled() const noexcept {
    return limiter_.enabled();
}

void RateLimitPlugin::setMaxRpm(int rpm) noexcept {
    limiter_.setMaxRpm(rpm);
}

core::RateLimitInfo RateLimitPlugin::getInfo(const std::string& id) {
    auto r = limiter_.getInfo(id);
    return {r.limit, r.remaining, r.reset_ms};
}

bool RateLimitPlugin::allow(const std::string& id, int custom_rpm) {
    return limiter_.allow(id, custom_rpm);
}
