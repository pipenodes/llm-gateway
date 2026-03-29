#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <json/json.h>
#include "core_services.h"

struct PluginRequestContext {
    std::string request_id;
    std::string key_alias;
    std::string client_ip;
    std::string model;
    std::string method;
    std::string path;
    bool is_stream = false;

    std::unordered_map<std::string, std::string> metadata;
};

enum class PluginAction {
    Continue,
    Block,
    Skip
};

struct PluginResult {
    PluginAction action = PluginAction::Continue;
    int error_code = 0;
    std::string error_message;

    std::string cached_response;
    std::unordered_map<std::string, std::string> response_headers;
};

class Plugin {
public:
    virtual ~Plugin() = default;

    [[nodiscard]] virtual std::string name() const = 0;
    [[nodiscard]] virtual std::string version() const = 0;

    virtual bool init(const Json::Value& config) = 0;

    virtual PluginResult before_request(
        Json::Value& body, PluginRequestContext& ctx) = 0;

    virtual PluginResult after_response(
        Json::Value& response, PluginRequestContext& ctx) = 0;

    virtual void shutdown() {}

    virtual Json::Value stats() const { return {}; }

    Plugin(const Plugin&) = delete;
    Plugin& operator=(const Plugin&) = delete;

protected:
    Plugin() = default;
};

class PluginManager {
public:
    PluginManager() = default;
    ~PluginManager();

    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    void init(const Json::Value& config);

    template<typename T>
    void register_builtin(const Json::Value& plug_config = Json::Value()) {
        auto plugin = std::make_unique<T>();
        Json::Value cfg = plug_config.isObject() ? plug_config
            : (config_.isMember(plugin->name()) ? config_[plugin->name()] : Json::Value());
        if (!plugin->init(cfg)) return;
        Plugin* raw = plugin.get();
        plugins_.push_back(std::move(plugin));
        update_services(raw);
    }

    void register_plugin(std::unique_ptr<Plugin> plugin);
    void register_plugin(std::unique_ptr<Plugin> plugin, bool already_inited);

    PluginResult run_before_request(
        Json::Value& body, PluginRequestContext& ctx);

    PluginResult run_after_response(
        Json::Value& response, PluginRequestContext& ctx);

    /** Notify all plugins implementing IAuditSink (thread-safe, called from request thread). */
    void notify_request_completed(const core::AuditEntry& entry);

    [[nodiscard]] Json::Value list_plugins() const;

    [[nodiscard]] Json::Value plugin_metrics() const;

    void shutdown_all();

    [[nodiscard]] bool enabled() const noexcept { return enabled_; }

    core::ICache* get_cache() const noexcept { return cache_service_; }
    core::IAuth* get_auth() const noexcept { return auth_service_; }
    core::IRateLimiter* get_rate_limiter() const noexcept { return rate_limiter_service_; }
    core::IRequestQueue* get_request_queue() const noexcept { return queue_service_; }
    core::IAuditQueryProvider* get_audit_query_provider() const noexcept { return audit_query_provider_; }

    template<typename T>
    T* find_plugin() const noexcept {
        for (const auto& p : plugins_) {
            if (auto* cast = dynamic_cast<T*>(p.get())) return cast;
        }
        return nullptr;
    }

private:
    void update_services(Plugin* p) noexcept;

    std::vector<std::unique_ptr<Plugin>> plugins_;
    Json::Value config_;
    bool enabled_ = false;
    core::ICache* cache_service_ = nullptr;
    core::IAuth* auth_service_ = nullptr;
    core::IRateLimiter* rate_limiter_service_ = nullptr;
    core::IRequestQueue* queue_service_ = nullptr;
    core::IAuditQueryProvider* audit_query_provider_ = nullptr;
};
