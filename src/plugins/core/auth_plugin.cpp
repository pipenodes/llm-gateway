#include "auth_plugin.h"

bool AuthPlugin::init(const Json::Value& config) {
    std::string path = config.get("keys_file", "keys.json").asString();
    keys_.init(path);
    return true;
}

PluginResult AuthPlugin::before_request(Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult AuthPlugin::after_response(Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}

bool AuthPlugin::hasActiveKeys() const {
    return keys_.hasActiveKeys();
}

size_t AuthPlugin::activeKeyCount() const {
    return keys_.activeKeyCount();
}

std::optional<core::AuthResult> AuthPlugin::validate(const std::string& bearer_token) {
    auto r = keys_.validate(bearer_token);
    if (!r) return std::nullopt;
    return core::AuthResult{r->alias, r->rate_limit_rpm, r->ip_whitelist};
}

core::AuthCreateResult AuthPlugin::create(const std::string& alias,
    int rate_limit_rpm,
    const std::vector<std::string>& ip_whitelist) {
    auto r = keys_.create(alias, rate_limit_rpm, ip_whitelist);
    core::AuthCreateResult out;
    out.ok = r.ok;
    out.error = r.error;
    out.key = r.key;
    out.alias = r.alias;
    out.created_at = r.created_at;
    return out;
}

bool AuthPlugin::revoke(const std::string& alias) {
    return keys_.revoke(alias);
}

Json::Value AuthPlugin::list() const {
    return keys_.list();
}
