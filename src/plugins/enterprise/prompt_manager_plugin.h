#pragma once
#include "../plugin.h"
#include <unordered_map>
#include <shared_mutex>
#include <string>
#include <vector>
#include <regex>
#include <cstdint>

struct PromptTemplate {
    std::string name;
    std::string content;
    std::vector<std::string> variables;
    int64_t created_at = 0;
    int version = 1;
};

struct GuardrailRule {
    std::string name;
    std::string pattern;
    std::string action;   // "block" | "warn" | "redact"
    std::string message;
    std::regex compiled;
};

class PromptManagerPlugin : public Plugin {
public:
    PromptManagerPlugin() = default;

    std::string name() const override { return "prompt_manager"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    void shutdown() override;

    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;

    Json::Value stats() const override;

    void set_key_system_prompt(const std::string& alias, const std::string& prompt);
    bool add_template(const std::string& name, const std::string& content,
                      const std::vector<std::string>& variables);
    [[nodiscard]] Json::Value list_templates() const;
    [[nodiscard]] Json::Value list_guardrails() const;

private:
    struct GuardrailResult {
        bool allowed = true;
        std::string reason;
        std::string rule_name;
        std::string action;
    };

    void inject_system_prompt(Json::Value& body, const std::string& key_alias) const;
    bool apply_template(Json::Value& body, const std::string& template_name,
                        const Json::Value& variables) const;
    [[nodiscard]] GuardrailResult check_guardrails(const Json::Value& messages) const;
    void apply_redactions(Json::Value& messages) const;
    [[nodiscard]] static std::string resolve_template(const std::string& content,
                                                       const Json::Value& vars);
    void load();
    void save() const;

    std::string persist_path_;
    std::string default_system_prompt_;
    std::string system_prompt_mode_ = "prepend"; // prepend | append | replace

    std::unordered_map<std::string, std::string> key_prompts_;
    std::unordered_map<std::string, PromptTemplate> templates_;
    std::vector<GuardrailRule> guardrails_;
    bool guardrails_enabled_ = true;

    mutable std::shared_mutex mtx_;
};
