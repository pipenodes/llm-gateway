#pragma once
#include "plugin.h"
#include "core_services.h"
#include "api_keys.h"
#include <memory>

class AuthPlugin : public Plugin, public core::IAuth {
public:
    std::string name() const override { return "auth"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;

    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;

    [[nodiscard]] bool hasActiveKeys() const override;
    [[nodiscard]] size_t activeKeyCount() const override;
    [[nodiscard]] std::optional<core::AuthResult> validate(const std::string& bearer_token) override;
    [[nodiscard]] core::AuthCreateResult create(const std::string& alias,
        int rate_limit_rpm = 0,
        const std::vector<std::string>& ip_whitelist = {}) override;
    [[nodiscard]] bool revoke(const std::string& alias) override;
    [[nodiscard]] Json::Value list() const override;

private:
    ApiKeyManager keys_;
};
