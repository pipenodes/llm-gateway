#include "plugin.h"
#include "logger.h"
#include <stdexcept>

PluginManager::~PluginManager() {
    shutdown_all();
}

void PluginManager::init(const Json::Value& config) {
    config_ = config;
    enabled_ = config.get("enabled", false).asBool();
}

void PluginManager::register_plugin(std::unique_ptr<Plugin> plugin) {
    register_plugin(std::move(plugin), false);
}

void PluginManager::register_plugin(std::unique_ptr<Plugin> plugin, bool already_inited) {
    if (!plugin) return;
    if (!already_inited
        && !plugin->init(config_.isMember(plugin->name()) ? config_[plugin->name()] : Json::Value())) {
        Json::Value f;
        f["plugin"] = plugin->name();
        Logger::warn("plugin_init_failed", f);
        return;
    }
    Plugin* raw = plugin.get();
    plugins_.push_back(std::move(plugin));
    update_services(raw);
}

void PluginManager::update_services(Plugin* p) noexcept {
    if (!p) return;
    if (auto* c = dynamic_cast<core::ICache*>(p)) cache_service_ = c;
    if (auto* a = dynamic_cast<core::IAuth*>(p)) auth_service_ = a;
    if (auto* r = dynamic_cast<core::IRateLimiter*>(p)) rate_limiter_service_ = r;
    if (auto* q = dynamic_cast<core::IRequestQueue*>(p)) queue_service_ = q;
    if (auto* aq = dynamic_cast<core::IAuditQueryProvider*>(p)) audit_query_provider_ = aq;
}

PluginResult PluginManager::run_before_request(
    Json::Value& body, PluginRequestContext& ctx) {
    if (!enabled_ || plugins_.empty())
        return PluginResult{.action = PluginAction::Continue};

    for (auto& plugin : plugins_) {
        try {
            PluginResult r = plugin->before_request(body, ctx);
            if (r.action == PluginAction::Block)
                return r;
            if (r.action == PluginAction::Skip)
                return PluginResult{.action = PluginAction::Continue};
        } catch (const std::exception& e) {
            Json::Value f;
            f["plugin"] = plugin->name();
            f["error"] = e.what();
            Logger::error("plugin_before_request_error", f);
            return PluginResult{.action = PluginAction::Block, .error_code = 500, .error_message = "Plugin error"};
        }
    }
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult PluginManager::run_after_response(
    Json::Value& response, PluginRequestContext& ctx) {
    if (!enabled_ || plugins_.empty())
        return PluginResult{.action = PluginAction::Continue};

    for (auto it = plugins_.rbegin(); it != plugins_.rend(); ++it) {
        try {
            PluginResult r = (*it)->after_response(response, ctx);
            if (r.action == PluginAction::Block)
                return r;
        } catch (const std::exception& e) {
            Json::Value f;
            f["plugin"] = (*it)->name();
            f["error"] = e.what();
            Logger::error("plugin_after_response_error", f);
        }
    }
    return PluginResult{.action = PluginAction::Continue};
}

void PluginManager::notify_request_completed(const core::AuditEntry& entry) {
    for (auto& plugin : plugins_) {
        if (auto* sink = dynamic_cast<core::IAuditSink*>(plugin.get())) {
            try {
                sink->on_request_completed(entry);
            } catch (const std::exception& e) {
                Json::Value f;
                f["plugin"] = plugin->name();
                f["error"] = e.what();
                Logger::error("audit_sink_error", f);
            }
        }
    }
}

Json::Value PluginManager::list_plugins() const {
    Json::Value out(Json::arrayValue);
    int pos = 0;
    for (const auto& p : plugins_) {
        Json::Value e;
        e["name"] = p->name();
        e["version"] = p->version();
        e["type"] = "builtin";
        e["enabled"] = true;
        e["position"] = pos++;
        out.append(e);
    }
    return out;
}

Json::Value PluginManager::plugin_metrics() const {
    Json::Value out(Json::objectValue);
    for (const auto& p : plugins_) {
        Json::Value s = p->stats();
        if (s.isObject() && !s.empty())
            out[p->name()] = s;
    }
    return out;
}

void PluginManager::shutdown_all() {
    for (auto& p : plugins_) {
        try {
            p->shutdown();
        } catch (const std::exception& e) {
            Json::Value f;
            f["plugin"] = p->name();
            f["error"] = e.what();
            Logger::error("plugin_shutdown_error", f);
        }
    }
}
