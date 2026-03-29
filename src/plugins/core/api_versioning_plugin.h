#pragma once
#include "../plugin.h"
#include <string>

class APIVersioningPlugin : public Plugin {
public:
    std::string name() const override { return "api_versioning"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;

private:
    std::string default_version_ = "v1";
};
