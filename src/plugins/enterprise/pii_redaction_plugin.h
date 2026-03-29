#pragma once
#include "../plugin.h"
#include <regex>
#include <vector>
#include <unordered_map>
#include <mutex>

struct PIIPattern {
    std::string name;
    std::regex pattern;
    std::string placeholder_prefix;
};

class PIIRedactionPlugin : public Plugin {
public:
    std::string name() const override { return "pii_redactor"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;

private:
    std::vector<PIIPattern> patterns_;
    bool log_redactions_ = true;
    std::string redact_text(const std::string& text, std::unordered_map<std::string, std::string>& map);
    std::string restore_text(const std::string& text, const std::unordered_map<std::string, std::string>& map) const;
    void redact_messages(Json::Value& messages, std::unordered_map<std::string, std::string>& map);
    static std::string serialize_map(const std::unordered_map<std::string, std::string>& map);
    static bool deserialize_map(const std::string& s, std::unordered_map<std::string, std::string>& map);
};
