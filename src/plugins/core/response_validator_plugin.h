#pragma once
#include "../plugin.h"
#include <string>
#include <vector>
#include <regex>

struct ValidationRule {
    std::string name;
    enum Type { RegexBlock, RegexRequire, MaxLength } type;
    std::string value;
    std::string action; // "block", "redact", "warn"
    std::regex compiled;
};

struct DisclaimerRule {
    std::regex trigger;
    std::string text;
};

class ResponseValidatorPlugin : public Plugin {
public:
    std::string name() const override { return "response_validator"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;

private:
    bool validate_json_mode_ = true;
    int max_content_length_ = 0;
    std::string fallback_message_ = "Invalid response";
    std::vector<ValidationRule> rules_;
    std::vector<DisclaimerRule> disclaimers_;

    struct ValidationResult {
        bool valid = true;
        std::string failed_rule;
        std::string details;
    };

    [[nodiscard]] ValidationResult validate_content(const std::string& content) const;
    [[nodiscard]] static bool is_valid_json(const std::string& content);
    std::string apply_redactions(const std::string& content) const;
    void add_disclaimers(std::string& content) const;
};
