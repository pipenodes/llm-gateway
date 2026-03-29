#include "request_router_plugin.h"

bool RequestRouterPlugin::init(const Json::Value& config) {
    default_model_ = config.get("default_model", "llama3:8b").asString();
    if (config.isMember("routes") && config["routes"].isObject()) {
        for (const auto& k : config["routes"].getMemberNames())
            route_rules_[k] = config["routes"][k].asString();
    }
    return true;
}

std::string RequestRouterPlugin::resolve_model(const Json::Value& body) const {
    std::string requested = body.get("model", "").asString();
    if (requested != "auto" && !requested.empty()) return requested;
    if (body.isMember("messages") && body["messages"].isArray() && body["messages"].size() > 0) {
        const auto& last = body["messages"][body["messages"].size() - 1];
        if (last.isMember("content") && last["content"].isString()) {
            std::string content = last["content"].asString();
            for (const auto& [k, model] : route_rules_) {
                if (content.find(k) != std::string::npos) return model;
            }
        }
    }
    return default_model_;
}

PluginResult RequestRouterPlugin::before_request(Json::Value& body, PluginRequestContext&) {
    std::string model = body.get("model", "").asString();
    if (model == "auto" || model.empty()) {
        body["model"] = resolve_model(body);
    }
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult RequestRouterPlugin::after_response(Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}
