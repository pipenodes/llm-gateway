#pragma once
#include "plugin.h"
#include <regex>
#include <vector>
#include <string>
#include <unordered_set>

class ContentModerationPlugin : public Plugin {
public:
    std::string name() const override { return "content_moderation"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;

private:
    std::vector<std::regex> block_patterns_;
    std::unordered_set<std::string> block_words_;
    std::string block_message_ = "Content blocked by policy";
    bool check_input_ = true;
    bool check_output_ = true;
    bool check(const std::string& text) const;
    std::string extract_content_from_messages(const Json::Value& body) const;
    std::string extract_content_from_response(const Json::Value& response) const;
};
