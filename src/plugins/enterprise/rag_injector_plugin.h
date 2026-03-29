#pragma once
#include "../plugin.h"
#include <string>
#include <vector>

class RAGInjectorPlugin : public Plugin {
public:
    std::string name() const override { return "rag_injector"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;

private:
    std::string context_prefix_ = "Context:\n";
    std::string context_suffix_ = "\n\n";
    int max_chars_ = 2000;
    void inject_context(Json::Value& messages, const std::string& context) const;
};
