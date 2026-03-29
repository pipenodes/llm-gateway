#pragma once
#include "../plugin.h"
#include <regex>
#include <vector>
#include <string>

struct InjectionPattern {
    std::string name;
    std::regex pattern;
    float weight;
};

class PromptInjectionPlugin : public Plugin {
public:
    std::string name() const override { return "prompt_injection"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;

private:
    std::vector<InjectionPattern> patterns_;
    float block_threshold_ = 0.5f;
    float score_prompt(const std::string& text) const;
    std::string extract_full_prompt(const Json::Value& body) const;
};
