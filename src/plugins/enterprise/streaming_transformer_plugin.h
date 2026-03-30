#pragma once
#include "plugin.h"
#include <string>
#include <vector>
#include <regex>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <atomic>

struct StreamTransform {
    enum Type { RegexReplace, AppendSuffix } type;
    std::regex pattern;
    std::string replacement;
    std::string suffix;
};

class StreamingTransformerPlugin : public Plugin {
public:
    std::string name() const override { return "streaming_transformer"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;
    Json::Value stats() const override;

    [[nodiscard]] bool supports_streaming() const noexcept { return true; }

    [[nodiscard]] std::string transform_chunk(const std::string& chunk,
                                               const std::string& request_id);
    [[nodiscard]] std::string flush_buffer(const std::string& request_id);

    void on_stream_start(const std::string& request_id);
    void on_stream_end(const std::string& request_id);

private:
    struct StreamMetrics {
        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point first_chunk_time;
        int chunk_count = 0;
        int total_chars = 0;
        bool first_chunk_received = false;
        std::string buffer;
    };

    std::vector<StreamTransform> transforms_;
    bool collect_metrics_ = true;
    bool log_ttft_ = true;
    bool buffer_partial_words_ = true;

    mutable std::mutex metrics_mtx_;
    std::unordered_map<std::string, StreamMetrics> active_streams_;

    std::atomic<int64_t> total_streams_{0};
    std::atomic<double> avg_ttft_ms_{0};
    std::atomic<double> avg_tokens_per_sec_{0};
};
