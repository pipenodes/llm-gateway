#include "response_validator_plugin.h"
#include "../logger.h"
#include <json/json.h>

bool ResponseValidatorPlugin::init(const Json::Value& config) {
    validate_json_mode_ = config.get("validate_json_mode", true).asBool();
    max_content_length_ = config.get("max_content_length", 0).asInt();
    fallback_message_ = config.get("fallback_message", "Invalid response").asString();

    if (config.isMember("rules") && config["rules"].isArray()) {
        for (const auto& r : config["rules"]) {
            ValidationRule rule;
            rule.name = r.get("name", "").asString();
            rule.value = r.get("value", "").asString();
            rule.action = r.get("action", "warn").asString();

            std::string type_str = r.get("type", "").asString();
            if (type_str == "regex_block") rule.type = ValidationRule::RegexBlock;
            else if (type_str == "regex_require") rule.type = ValidationRule::RegexRequire;
            else if (type_str == "max_length") rule.type = ValidationRule::MaxLength;
            else continue;

            if (rule.type == ValidationRule::RegexBlock || rule.type == ValidationRule::RegexRequire) {
                try {
                    rule.compiled = std::regex(rule.value, std::regex::icase | std::regex::optimize);
                } catch (const std::regex_error& e) {
                    Json::Value f;
                    f["rule"] = rule.name;
                    f["error"] = e.what();
                    Logger::warn("validator_regex_error", f);
                    continue;
                }
            }
            rules_.push_back(std::move(rule));
        }
    }

    if (config.isMember("disclaimers") && config["disclaimers"].isArray()) {
        for (const auto& d : config["disclaimers"]) {
            std::string trigger = d.get("trigger", "").asString();
            std::string text = d.get("text", "").asString();
            if (trigger.empty() || text.empty()) continue;
            try {
                disclaimers_.push_back({
                    std::regex(trigger, std::regex::icase | std::regex::optimize),
                    text
                });
            } catch (const std::regex_error&) {}
        }
    }

    return true;
}

PluginResult ResponseValidatorPlugin::before_request(Json::Value& body, PluginRequestContext& ctx) {
    if (body.isMember("response_format") && body["response_format"].isObject()) {
        std::string type = body["response_format"].get("type", "").asString();
        if (type == "json_object")
            ctx.metadata["response_format_json"] = "true";
    }
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult ResponseValidatorPlugin::after_response(Json::Value& response, PluginRequestContext& ctx) {
    if (!response.isMember("choices") || !response["choices"].isArray() || response["choices"].empty())
        return PluginResult{.action = PluginAction::Continue};

    auto& choice = response["choices"][0];
    if (!choice.isMember("message") || !choice["message"].isMember("content"))
        return PluginResult{.action = PluginAction::Continue};

    std::string content = choice["message"]["content"].asString();

    if (validate_json_mode_ && ctx.metadata.count("response_format_json")) {
        if (!is_valid_json(content)) {
            Json::Value f;
            f["request_id"] = ctx.request_id;
            f["error"] = "invalid_json_response";
            Logger::warn("response_validator_json_invalid", f);
            choice["message"]["content"] = fallback_message_;
            return PluginResult{.action = PluginAction::Continue};
        }
    }

    auto result = validate_content(content);
    if (!result.valid) {
        Json::Value f;
        f["rule"] = result.failed_rule;
        f["details"] = result.details;
        f["request_id"] = ctx.request_id;
        Logger::warn("response_validator_failed", f);

        choice["message"]["content"] = fallback_message_;
        return PluginResult{.action = PluginAction::Continue};
    }

    content = apply_redactions(content);
    add_disclaimers(content);
    choice["message"]["content"] = content;

    return PluginResult{.action = PluginAction::Continue};
}

ResponseValidatorPlugin::ValidationResult
ResponseValidatorPlugin::validate_content(const std::string& content) const {
    if (max_content_length_ > 0 && static_cast<int>(content.size()) > max_content_length_) {
        return {false, "max_content_length", "content exceeds max_length"};
    }

    for (const auto& rule : rules_) {
        if (rule.type == ValidationRule::RegexRequire) {
            if (!std::regex_search(content, rule.compiled)) {
                if (rule.action == "block")
                    return {false, rule.name, "required pattern not found"};
            }
        }
        if (rule.type == ValidationRule::MaxLength) {
            try {
                int limit = std::stoi(rule.value);
                if (static_cast<int>(content.size()) > limit) {
                    if (rule.action == "block")
                        return {false, rule.name, "exceeds rule max_length"};
                    if (rule.action == "warn") {
                        Json::Value f;
                        f["rule"] = rule.name;
                        f["length"] = static_cast<int>(content.size());
                        Logger::warn("content_length_warning", f);
                    }
                }
            } catch (...) {}
        }
        if (rule.type == ValidationRule::RegexBlock && rule.action == "block") {
            if (std::regex_search(content, rule.compiled))
                return {false, rule.name, "blocked pattern found"};
        }
    }
    return {true, "", ""};
}

bool ResponseValidatorPlugin::is_valid_json(const std::string& content) {
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
    return reader->parse(content.data(), content.data() + content.size(), &root, &errs);
}

std::string ResponseValidatorPlugin::apply_redactions(const std::string& content) const {
    std::string result = content;
    for (const auto& rule : rules_) {
        if (rule.type == ValidationRule::RegexBlock && rule.action == "redact") {
            result = std::regex_replace(result, rule.compiled, "[REDACTED]");
        }
    }
    return result;
}

void ResponseValidatorPlugin::add_disclaimers(std::string& content) const {
    for (const auto& d : disclaimers_) {
        if (std::regex_search(content, d.trigger))
            content += d.text;
    }
}
