#include "content_moderation_plugin.h"
#include "../logger.h"
#include <sstream>
#include <algorithm>
#include <cctype>

bool ContentModerationPlugin::init(const Json::Value& config) {
    block_message_ = config.get("block_message", "Content blocked by policy").asString();
    check_input_ = config.get("check_input", true).asBool();
    check_output_ = config.get("check_output", true).asBool();
    if (config.isMember("block_words") && config["block_words"].isArray()) {
        for (const auto& w : config["block_words"])
            block_words_.insert(w.asString());
    }
    if (config.isMember("block_patterns") && config["block_patterns"].isArray()) {
        for (const auto& p : config["block_patterns"]) {
            try {
                block_patterns_.emplace_back(p.asString());
            } catch (const std::regex_error&) {}
        }
    }
    return true;
}

bool ContentModerationPlugin::check(const std::string& text) const {
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    for (const auto& w : block_words_) {
        if (lower.find(w) != std::string::npos) return true;
    }
    for (const auto& re : block_patterns_) {
        if (std::regex_search(text, re)) return true;
    }
    return false;
}

std::string ContentModerationPlugin::extract_content_from_messages(const Json::Value& body) const {
    std::ostringstream os;
    if (body.isMember("messages") && body["messages"].isArray()) {
        for (const auto& m : body["messages"]) {
            if (m.isMember("content") && m["content"].isString())
                os << m["content"].asString() << " ";
        }
    }
    return os.str();
}

std::string ContentModerationPlugin::extract_content_from_response(const Json::Value& response) const {
    if (response.isMember("choices") && response["choices"].isArray() && response["choices"].size() > 0) {
        const auto& c = response["choices"][0];
        if (c.isMember("message") && c["message"].isMember("content") && c["message"]["content"].isString())
            return c["message"]["content"].asString();
    }
    return {};
}

PluginResult ContentModerationPlugin::before_request(Json::Value& body, PluginRequestContext&) {
    if (!check_input_) return PluginResult{.action = PluginAction::Continue};
    std::string content = extract_content_from_messages(body);
    if (check(content)) {
        Json::Value f;
        f["request_id"] = "content_moderation";
        Logger::warn("content_moderation_blocked_input", f);
        return PluginResult{.action = PluginAction::Block, .error_code = 400, .error_message = block_message_};
    }
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult ContentModerationPlugin::after_response(Json::Value& response, PluginRequestContext&) {
    if (!check_output_) return PluginResult{.action = PluginAction::Continue};
    std::string content = extract_content_from_response(response);
    if (check(content)) {
        Json::Value f;
        Logger::warn("content_moderation_blocked_output", f);
        if (response.isMember("choices") && response["choices"].isArray() && response["choices"].size() > 0) {
            response["choices"][0]["message"]["content"] = block_message_;
        }
    }
    return PluginResult{.action = PluginAction::Continue};
}
