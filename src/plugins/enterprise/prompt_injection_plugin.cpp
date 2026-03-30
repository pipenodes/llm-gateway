#include "prompt_injection_plugin.h"
#include "logger.h"
#include <sstream>
#include <string>
#include <tuple>

bool PromptInjectionPlugin::init(const Json::Value& config) {
    block_threshold_ = static_cast<float>(config.get("block_threshold", 0.5).asDouble());
    patterns_.clear();
    std::vector<std::tuple<std::string, std::string, float>> builtin = {
        {"ignore_instructions", R"((?i)(ignore|disregard|forget)\s+(all\s+)?(previous|above|prior)\s+instructions)", 0.9f},
        {"role_override", R"((?i)you\s+are\s+now\s+(DAN|jailbreak|without\s+restrictions))", 0.85f},
        {"extract_system", R"((?i)(reveal|show|output|print)\s+(your|the)\s+(system\s+)?(prompt|instructions))", 0.9f},
        {"new_instruction", R"((?i)(new\s+instruction|following\s+instruction|do\s+this\s+instead):)", 0.4f},
    };
    for (const auto& [name, re, w] : builtin) {
        try {
            patterns_.push_back({name, std::regex(re), w});
        } catch (const std::exception&) {}
    }
    if (config.isMember("patterns") && config["patterns"].isArray()) {
        for (const auto& p : config["patterns"]) {
            if (p.isMember("name") && p.isMember("regex") && p.isMember("weight")) {
                try {
                    patterns_.push_back({
                        p["name"].asString(),
                        std::regex(p["regex"].asString()),
                        static_cast<float>(p["weight"].asDouble())
                    });
                } catch (const std::regex_error&) {}
            }
        }
    }
    return true;
}

float PromptInjectionPlugin::score_prompt(const std::string& text) const {
    float score = 0.f;
    for (const auto& pat : patterns_) {
        if (std::regex_search(text, pat.pattern))
            score += pat.weight;
    }
    return score > 1.f ? 1.f : score;
}

std::string PromptInjectionPlugin::extract_full_prompt(const Json::Value& body) const {
    std::ostringstream os;
    if (body.isMember("messages") && body["messages"].isArray()) {
        for (const auto& m : body["messages"]) {
            if (m.isMember("content") && m["content"].isString())
                os << m["content"].asString() << "\n";
        }
    }
    return os.str();
}

PluginResult PromptInjectionPlugin::before_request(Json::Value& body, PluginRequestContext& ctx) {
    std::string prompt = extract_full_prompt(body);
    float s = score_prompt(prompt);
    if (s >= block_threshold_) {
        Json::Value f;
        f["score"] = s;
        f["request_id"] = ctx.request_id;
        Logger::warn("prompt_injection_blocked", f);
        return PluginResult{.action = PluginAction::Block, .error_code = 400,
            .error_message = "Request blocked: possible prompt injection detected"};
    }
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult PromptInjectionPlugin::after_response(Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}
