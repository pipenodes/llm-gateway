#include "adaptive_rate_limiter_plugin.h"
#include "logger.h"
#include "crypto.h"
#include <algorithm>
#include <cmath>

bool AdaptiveRateLimiterPlugin::init(const Json::Value& config) {
    latency_warning_ms_ = config.get("latency_warning_ms", 5000).asDouble();
    latency_critical_ms_ = config.get("latency_critical_ms", 15000).asDouble();
    error_rate_warning_ = config.get("error_rate_warning", 0.1).asDouble();
    error_rate_critical_ = config.get("error_rate_critical", 0.5).asDouble();
    circuit_open_seconds_ = config.get("circuit_open_seconds", 30).asInt();
    circuit_failure_threshold_ = config.get("circuit_failure_threshold", 5).asInt();
    recovery_step_ = config.get("recovery_step", 0.25).asDouble();
    window_seconds_ = config.get("window_seconds", 60).asInt();
    default_rpm_ = config.get("default_rpm", 60).asInt();

    Logger::info("adaptive_rate_limiter_init");
    return true;
}

std::string AdaptiveRateLimiterPlugin::resolve_backend(const PluginRequestContext& ctx) const {
    auto it = ctx.metadata.find("backend");
    if (it != ctx.metadata.end()) return it->second;
    return "default";
}

PluginResult AdaptiveRateLimiterPlugin::before_request(Json::Value&, PluginRequestContext& ctx) {
    std::string backend = resolve_backend(ctx);
    int64_t now = static_cast<int64_t>(crypto::epoch_seconds());

    std::unique_lock lock(mtx_);
    auto& h = backends_[backend];

    if (h.state == CircuitState::Open) {
        if (now >= h.circuit_open_until) {
            h.state = CircuitState::HalfOpen;
            Json::Value f;
            f["backend"] = backend;
            Logger::info("circuit_half_open", f);
        } else {
            return PluginResult{
                .action = PluginAction::Block,
                .error_code = 503,
                .error_message = "Backend circuit breaker open"
            };
        }
    }

    ctx.metadata["_arl_start_ms"] = std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    ctx.metadata["_arl_backend"] = backend;

    return PluginResult{.action = PluginAction::Continue};
}

PluginResult AdaptiveRateLimiterPlugin::after_response(Json::Value& response, PluginRequestContext& ctx) {
    auto start_it = ctx.metadata.find("_arl_start_ms");
    if (start_it == ctx.metadata.end())
        return PluginResult{.action = PluginAction::Continue};

    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    int64_t start_ms = std::stoll(start_it->second);
    double latency = static_cast<double>(now_ms - start_ms);

    bool is_error = !response.isMember("choices") && !response.isMember("data");
    std::string backend = ctx.metadata.count("_arl_backend")
        ? ctx.metadata["_arl_backend"] : "default";

    record_sample(backend, latency, is_error);

    return PluginResult{.action = PluginAction::Continue};
}

void AdaptiveRateLimiterPlugin::on_request_completed(const core::AuditEntry& entry) {
    bool is_error = entry.status_code >= 500;
    record_sample("default", static_cast<double>(entry.latency_ms), is_error);
}

void AdaptiveRateLimiterPlugin::record_sample(const std::string& backend,
                                                double latency_ms, bool error) {
    int64_t now = static_cast<int64_t>(crypto::epoch_seconds());

    std::unique_lock lock(mtx_);
    auto& h = backends_[backend];

    h.samples.push_back({now, latency_ms, error});
    prune_window(h);

    if (error) {
        h.consecutive_failures++;
        h.last_failure_at = now;

        if (h.consecutive_failures >= circuit_failure_threshold_ && h.state != CircuitState::Open) {
            h.state = CircuitState::Open;
            h.circuit_open_until = now + circuit_open_seconds_;
            h.rate_multiplier = 0.0;
            Json::Value f;
            f["backend"] = backend;
            f["failures"] = h.consecutive_failures;
            Logger::warn("circuit_opened", f);
        }
    } else {
        if (h.state == CircuitState::HalfOpen) {
            h.state = CircuitState::Closed;
            h.rate_multiplier = recovery_step_;
            h.consecutive_failures = 0;
            Json::Value f;
            f["backend"] = backend;
            Logger::info("circuit_closed_recovery", f);
        } else {
            h.consecutive_failures = 0;
        }
    }

    update_health(h);
}

void AdaptiveRateLimiterPlugin::update_health(BackendHealth& h) const {
    if (h.state == CircuitState::Open) return;

    double error_rate = compute_error_rate(h);
    double p95 = compute_p95(h);

    if (error_rate >= error_rate_critical_ || p95 >= latency_critical_ms_) {
        h.rate_multiplier = std::max(0.1, h.rate_multiplier - recovery_step_);
    } else if (error_rate >= error_rate_warning_ || p95 >= latency_warning_ms_) {
        h.rate_multiplier = std::max(0.25, h.rate_multiplier - recovery_step_ * 0.5);
    } else if (h.rate_multiplier < 1.0) {
        h.rate_multiplier = std::min(1.0, h.rate_multiplier + recovery_step_);
    }
}

void AdaptiveRateLimiterPlugin::prune_window(BackendHealth& h) const {
    int64_t cutoff = static_cast<int64_t>(crypto::epoch_seconds()) - window_seconds_;
    while (!h.samples.empty() && h.samples.front().timestamp < cutoff)
        h.samples.pop_front();
}

double AdaptiveRateLimiterPlugin::compute_p95(const BackendHealth& h) const {
    if (h.samples.empty()) return 0;
    std::vector<double> latencies;
    latencies.reserve(h.samples.size());
    for (const auto& s : h.samples) latencies.push_back(s.latency_ms);
    std::sort(latencies.begin(), latencies.end());
    size_t idx = static_cast<size_t>(std::ceil(0.95 * latencies.size())) - 1;
    return latencies[std::min(idx, latencies.size() - 1)];
}

double AdaptiveRateLimiterPlugin::compute_error_rate(const BackendHealth& h) const {
    if (h.samples.empty()) return 0;
    int errors = 0;
    for (const auto& s : h.samples)
        if (s.error) errors++;
    return static_cast<double>(errors) / static_cast<double>(h.samples.size());
}

Json::Value AdaptiveRateLimiterPlugin::stats() const {
    std::shared_lock lock(mtx_);
    Json::Value out(Json::objectValue);
    Json::Value backends(Json::objectValue);

    for (const auto& [name, h] : backends_) {
        Json::Value b;
        switch (h.state) {
            case CircuitState::Closed: b["state"] = "closed"; break;
            case CircuitState::Open: b["state"] = "open"; break;
            case CircuitState::HalfOpen: b["state"] = "half_open"; break;
        }
        b["rate_multiplier"] = h.rate_multiplier;
        b["consecutive_failures"] = h.consecutive_failures;
        b["samples_in_window"] = static_cast<int>(h.samples.size());
        b["p95_latency_ms"] = compute_p95(h);
        b["error_rate"] = compute_error_rate(h);

        double sum = 0;
        for (const auto& s : h.samples) sum += s.latency_ms;
        b["avg_latency_ms"] = h.samples.empty() ? 0.0 : sum / h.samples.size();

        backends[name] = b;
    }

    out["backends"] = backends;
    out["default_rpm"] = default_rpm_;
    return out;
}
