#pragma once
#include "../plugin.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstdint>

struct WarmupModel {
    std::string name;
    int interval_seconds = 240;
    bool always_loaded = false;
    std::string schedule;
};

struct ModelStatus {
    std::string name;
    bool loaded = false;
    int64_t last_warmup = 0;
    int64_t last_used = 0;
    int warmup_count = 0;
    int cold_start_count = 0;
    double avg_load_time_ms = 0;
    double total_load_time_ms = 0;
};

class ModelWarmupPlugin : public Plugin {
public:
    ModelWarmupPlugin() = default;
    ~ModelWarmupPlugin() override;

    std::string name() const override { return "model_warmup"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    void shutdown() override;
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;
    Json::Value stats() const override;

private:
    void warmup_loop();
    void warmup_model(const WarmupModel& model);
    [[nodiscard]] bool is_within_schedule(const std::string& schedule) const;

    std::vector<WarmupModel> models_;
    mutable std::mutex mtx_;
    std::unordered_map<std::string, ModelStatus> status_;

    std::string ollama_host_ = "localhost";
    int ollama_port_ = 11434;
    std::string warmup_prompt_ = "Hi";
    int warmup_max_tokens_ = 1;
    bool startup_warmup_ = true;

    std::atomic<bool> running_{false};
    std::jthread warmup_thread_;
};
