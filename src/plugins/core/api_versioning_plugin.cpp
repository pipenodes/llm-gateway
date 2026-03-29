#include "api_versioning_plugin.h"

bool APIVersioningPlugin::init(const Json::Value& config) {
    default_version_ = config.get("default_version", "v1").asString();
    return true;
}

PluginResult APIVersioningPlugin::before_request(Json::Value& body, PluginRequestContext& ctx) {
    if (!ctx.metadata.count("api_version"))
        ctx.metadata["api_version"] = default_version_;
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult APIVersioningPlugin::after_response(Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}
