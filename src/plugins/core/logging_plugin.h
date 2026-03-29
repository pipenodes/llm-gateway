#pragma once
#include "../plugin.h"

class LoggingPlugin : public Plugin {
public:
    std::string name() const override { return "logging"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;

    PluginResult before_request(
        Json::Value& body, PluginRequestContext& ctx) override;

    PluginResult after_response(
        Json::Value& response, PluginRequestContext& ctx) override;
};
