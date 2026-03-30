#pragma once
#include "plugin.h"
#include <unordered_map>
#include <string>
#include <mutex>
#include <chrono>

class RequestDeduplicationPlugin : public Plugin {
public:
    std::string name() const override { return "request_dedup"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;

private:
    int window_seconds_ = 60;
    size_t max_entries_ = 1000;
    std::unordered_map<std::string, std::pair<std::string, std::chrono::steady_clock::time_point>> cache_;
    mutable std::mutex mtx_;
    std::string hash_request(const Json::Value& body) const;
    void cleanup_expired();
};
