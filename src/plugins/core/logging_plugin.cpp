#include "logging_plugin.h"
#include "logger.h"

bool LoggingPlugin::init(const Json::Value&) {
    return true;
}

PluginResult LoggingPlugin::before_request(
    Json::Value& /* body */, PluginRequestContext& ctx) {
    Json::Value f;
    f["request_id"] = ctx.request_id;
    f["model"] = ctx.model;
    f["path"] = ctx.path;
    if (!ctx.key_alias.empty()) f["key_alias"] = ctx.key_alias;
    Logger::info("plugin_before_request", f);
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult LoggingPlugin::after_response(
    Json::Value& /* response */, PluginRequestContext& ctx) {
    Json::Value f;
    f["request_id"] = ctx.request_id;
    f["model"] = ctx.model;
    Logger::info("plugin_after_response", f);
    return PluginResult{.action = PluginAction::Continue};
}
