#include "streaming_transformer_plugin.h"
#include "logger.h"

bool StreamingTransformerPlugin::init(const Json::Value& config) {
    collect_metrics_ = config.get("collect_metrics", true).asBool();
    log_ttft_ = config.get("log_ttft", true).asBool();
    buffer_partial_words_ = config.get("buffer_partial_words", true).asBool();

    if (config.isMember("transforms") && config["transforms"].isArray()) {
        for (const auto& t : config["transforms"]) {
            StreamTransform tr;
            std::string type = t.get("type", "").asString();

            if (type == "regex_replace") {
                tr.type = StreamTransform::RegexReplace;
                std::string pattern = t.get("pattern", "").asString();
                tr.replacement = t.get("replacement", "***").asString();
                try {
                    tr.pattern = std::regex(pattern, std::regex::optimize);
                    transforms_.push_back(std::move(tr));
                } catch (const std::regex_error& e) {
                    Json::Value f;
                    f["pattern"] = pattern;
                    f["error"] = e.what();
                    Logger::warn("stream_transform_regex_error", f);
                }
            } else if (type == "append_suffix") {
                tr.type = StreamTransform::AppendSuffix;
                tr.suffix = t.get("suffix", "").asString();
                transforms_.push_back(std::move(tr));
            }
        }
    }

    Json::Value f;
    f["transforms"] = static_cast<int>(transforms_.size());
    Logger::info("streaming_transformer_init", f);
    return true;
}

PluginResult StreamingTransformerPlugin::before_request(Json::Value& body, PluginRequestContext& ctx) {
    if (body.get("stream", false).asBool())
        on_stream_start(ctx.request_id);
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult StreamingTransformerPlugin::after_response(Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}

void StreamingTransformerPlugin::on_stream_start(const std::string& request_id) {
    if (!collect_metrics_) return;
    std::lock_guard lock(metrics_mtx_);
    auto& m = active_streams_[request_id];
    m.start_time = std::chrono::steady_clock::now();
    m.chunk_count = 0;
    m.total_chars = 0;
    m.first_chunk_received = false;
}

std::string StreamingTransformerPlugin::transform_chunk(const std::string& chunk,
                                                         const std::string& request_id) {
    if (collect_metrics_) {
        std::lock_guard lock(metrics_mtx_);
        auto it = active_streams_.find(request_id);
        if (it != active_streams_.end()) {
            auto& m = it->second;
            m.chunk_count++;
            m.total_chars += static_cast<int>(chunk.size());
            if (!m.first_chunk_received) {
                m.first_chunk_received = true;
                m.first_chunk_time = std::chrono::steady_clock::now();

                if (log_ttft_) {
                    auto ttft = std::chrono::duration_cast<std::chrono::milliseconds>(
                        m.first_chunk_time - m.start_time).count();
                    Json::Value f;
                    f["request_id"] = request_id;
                    f["ttft_ms"] = static_cast<Json::Int64>(ttft);
                    Logger::info("stream_ttft", f);
                }
            }
        }
    }

    if (transforms_.empty()) return chunk;

    std::string input = chunk;

    if (buffer_partial_words_) {
        std::lock_guard lock(metrics_mtx_);
        auto it = active_streams_.find(request_id);
        if (it != active_streams_.end() && !it->second.buffer.empty()) {
            input = it->second.buffer + input;
            it->second.buffer.clear();
        }
    }

    if (buffer_partial_words_ && !input.empty()) {
        auto last_space = input.find_last_of(" \n\t\r");
        if (last_space != std::string::npos && last_space < input.size() - 1) {
            std::string tail = input.substr(last_space + 1);
            input = input.substr(0, last_space + 1);
            std::lock_guard lock(metrics_mtx_);
            auto it = active_streams_.find(request_id);
            if (it != active_streams_.end())
                it->second.buffer = tail;
        }
    }

    std::string result = input;
    for (const auto& tr : transforms_) {
        if (tr.type == StreamTransform::RegexReplace)
            result = std::regex_replace(result, tr.pattern, tr.replacement);
        else if (tr.type == StreamTransform::AppendSuffix && !tr.suffix.empty())
            result += tr.suffix;
    }
    return result;
}

std::string StreamingTransformerPlugin::flush_buffer(const std::string& request_id) {
    std::lock_guard lock(metrics_mtx_);
    auto it = active_streams_.find(request_id);
    if (it == active_streams_.end()) return {};

    std::string remaining = std::move(it->second.buffer);
    it->second.buffer.clear();
    if (remaining.empty() || transforms_.empty()) return remaining;

    std::string result = remaining;
    for (const auto& tr : transforms_) {
        if (tr.type == StreamTransform::RegexReplace)
            result = std::regex_replace(result, tr.pattern, tr.replacement);
        else if (tr.type == StreamTransform::AppendSuffix && !tr.suffix.empty())
            result += tr.suffix;
    }
    return result;
}

void StreamingTransformerPlugin::on_stream_end(const std::string& request_id) {
    std::lock_guard lock(metrics_mtx_);
    auto it = active_streams_.find(request_id);
    if (it == active_streams_.end()) return;

    auto& m = it->second;

    if (!m.buffer.empty())
        m.buffer.clear();

    auto now = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m.start_time).count();

    if (m.first_chunk_received) {
        double ttft = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(
            m.first_chunk_time - m.start_time).count());
        int64_t streams = total_streams_.fetch_add(1) + 1;
        double old_avg = avg_ttft_ms_.load();
        avg_ttft_ms_.store(old_avg + (ttft - old_avg) / static_cast<double>(streams));

        if (duration_ms > 0) {
            int estimated_tokens = m.total_chars / 4;
            double tps = static_cast<double>(estimated_tokens) / (static_cast<double>(duration_ms) / 1000.0);
            double old_tps = avg_tokens_per_sec_.load();
            avg_tokens_per_sec_.store(old_tps + (tps - old_tps) / static_cast<double>(streams));
        }
    }

    active_streams_.erase(it);
}

Json::Value StreamingTransformerPlugin::stats() const {
    Json::Value out;
    out["transforms"] = static_cast<int>(transforms_.size());
    out["total_streams"] = static_cast<Json::Int64>(total_streams_.load());
    out["avg_ttft_ms"] = avg_ttft_ms_.load();
    out["avg_tokens_per_sec"] = avg_tokens_per_sec_.load();

    std::lock_guard lock(metrics_mtx_);
    out["active_streams"] = static_cast<int>(active_streams_.size());
    return out;
}
