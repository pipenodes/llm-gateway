#pragma once
#include "plugin.h"
#include <string>
#include <unordered_map>

class RequestRouterPlugin : public Plugin {
public:
    std::string name() const override { return "request_router"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;

private:
    std::unordered_map<std::string, std::string> route_rules_;
    std::string default_model_;
    std::string resolve_model(const Json::Value& body) const;
};
