#include "rag_injector_plugin.h"
#include <sstream>

bool RAGInjectorPlugin::init(const Json::Value& config) {
    context_prefix_ = config.get("context_prefix", "Context:\n").asString();
    context_suffix_ = config.get("context_suffix", "\n\n").asString();
    max_chars_ = config.get("max_context_chars", 2000).asInt();
    return true;
}

void RAGInjectorPlugin::inject_context(Json::Value& messages, const std::string& context) const {
    if (context.empty() || !messages.isArray() || messages.empty()) return;
    std::string trimmed = context;
    if (static_cast<int>(trimmed.size()) > max_chars_)
        trimmed = trimmed.substr(0, static_cast<size_t>(max_chars_)) + "...";
    std::string system_add = context_prefix_ + trimmed + context_suffix_;
    bool found_system = false;
    for (auto& m : messages) {
        if (m.isMember("role") && m["role"].asString() == "system") {
            m["content"] = m["content"].asString() + "\n" + system_add;
            found_system = true;
            break;
        }
    }
    if (!found_system) {
        Json::Value sys;
        sys["role"] = "system";
        sys["content"] = system_add;
        messages.insert(0, sys);
    }
}

PluginResult RAGInjectorPlugin::before_request(Json::Value& body, PluginRequestContext&) {
    if (!body.isMember("messages") || !body["messages"].isArray()) return PluginResult{.action = PluginAction::Continue};
    std::string context = body.get("rag_context", "").asString();
    if (!context.empty())
        inject_context(body["messages"], context);
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult RAGInjectorPlugin::after_response(Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}
