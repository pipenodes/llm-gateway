#include "ab_testing_plugin.h"
#include "../logger.h"
#include "../crypto.h"
#include <algorithm>
#include <cmath>
#include <numeric>

thread_local std::mt19937 ABTestingPlugin::rng_{std::random_device{}()};

bool ABTestingPlugin::init(const Json::Value& config) {
    if (config.isMember("experiments") && config["experiments"].isArray()) {
        for (const auto& e : config["experiments"]) {
            ABExperiment exp;
            exp.name = e.get("name", "").asString();
            exp.trigger_model = e.get("trigger_model", "").asString();
            exp.enabled = e.get("enabled", true).asBool();
            exp.created_at = crypto::epoch_seconds();

            if (e.isMember("variants") && e["variants"].isArray()) {
                int total_weight = 0;
                for (const auto& v : e["variants"]) {
                    ABVariant var;
                    var.model = v.get("model", "").asString();
                    var.provider = v.get("provider", "").asString();
                    var.weight = v.get("weight", 0).asInt();
                    total_weight += var.weight;
                    exp.variants.push_back(std::move(var));

                    ABVariantMetrics m;
                    m.model = exp.variants.back().model;
                    exp.metrics[m.model] = m;
                }

                if (exp.variants.size() >= 2 && exp.variants.size() <= 5
                    && total_weight == 100 && !exp.name.empty()) {
                    experiments_[exp.name] = std::move(exp);
                } else {
                    Json::Value f;
                    f["experiment"] = exp.name;
                    f["variants"] = static_cast<int>(exp.variants.size());
                    f["total_weight"] = total_weight;
                    Logger::warn("ab_experiment_invalid", f);
                }
            }
        }
    }

    Json::Value f;
    f["experiments"] = static_cast<int>(experiments_.size());
    Logger::info("ab_testing_init", f);
    return true;
}

std::optional<std::string> ABTestingPlugin::resolve(const std::string& requested_model,
                                                     const std::string& request_id) {
    std::unique_lock lock(mtx_);

    for (auto& [name, exp] : experiments_) {
        if (!exp.enabled) continue;
        if (exp.trigger_model != requested_model && exp.trigger_model != "default") continue;

        std::uniform_int_distribution<int> dist(1, 100);
        int roll = dist(rng_);
        int cumulative = 0;

        for (const auto& var : exp.variants) {
            cumulative += var.weight;
            if (roll <= cumulative) {
                request_to_experiment_[request_id] = name;
                request_to_variant_[request_id] = var.model;
                return var.model;
            }
        }

        if (!exp.variants.empty()) {
            const auto& last = exp.variants.back();
            request_to_experiment_[request_id] = name;
            request_to_variant_[request_id] = last.model;
            return last.model;
        }
    }

    return std::nullopt;
}

void ABTestingPlugin::record(const std::string& experiment_name, const std::string& variant_model,
                              int tokens, double latency_ms, bool error) {
    std::unique_lock lock(mtx_);
    auto it = experiments_.find(experiment_name);
    if (it == experiments_.end()) return;

    auto mit = it->second.metrics.find(variant_model);
    if (mit == it->second.metrics.end()) return;

    auto& m = mit->second;
    m.request_count++;
    m.total_tokens += tokens;
    m.total_latency_ms += latency_ms;
    if (error) m.error_count++;

    m.latencies.push_back(latency_ms);
    while (m.latencies.size() > MAX_LATENCY_SAMPLES)
        m.latencies.pop_front();
}

PluginResult ABTestingPlugin::before_request(Json::Value& body, PluginRequestContext& ctx) {
    if (!body.isMember("model")) return PluginResult{.action = PluginAction::Continue};

    std::string requested_model = body["model"].asString();
    auto resolved = resolve(requested_model, ctx.request_id);

    if (resolved) {
        body["model"] = *resolved;
        ctx.metadata["ab_variant"] = *resolved;
        ctx.metadata["ab_original_model"] = requested_model;
        ctx.metadata["X-AB-Variant"] = *resolved;
    }

    return PluginResult{.action = PluginAction::Continue};
}

PluginResult ABTestingPlugin::after_response(Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}

void ABTestingPlugin::on_request_completed(const core::AuditEntry& entry) {
    std::string exp_name, variant;
    {
        std::shared_lock lock(mtx_);
        auto eit = request_to_experiment_.find(entry.request_id);
        if (eit == request_to_experiment_.end()) return;
        exp_name = eit->second;

        auto vit = request_to_variant_.find(entry.request_id);
        if (vit == request_to_variant_.end()) return;
        variant = vit->second;
    }

    int tokens = entry.prompt_tokens + entry.completion_tokens;
    bool error = entry.status_code >= 400;
    record(exp_name, variant, tokens, static_cast<double>(entry.latency_ms), error);

    {
        std::unique_lock lock(mtx_);
        request_to_experiment_.erase(entry.request_id);
        request_to_variant_.erase(entry.request_id);
    }
}

Json::Value ABTestingPlugin::stats() const {
    std::shared_lock lock(mtx_);
    Json::Value out;
    out["experiments"] = static_cast<int>(experiments_.size());
    return out;
}

double ABTestingPlugin::compute_percentile(const std::deque<double>& vals, double pct) const {
    if (vals.empty()) return 0;
    std::vector<double> sorted(vals.begin(), vals.end());
    std::sort(sorted.begin(), sorted.end());
    size_t idx = static_cast<size_t>(std::ceil(pct * sorted.size())) - 1;
    return sorted[std::min(idx, sorted.size() - 1)];
}

Json::Value ABTestingPlugin::get_results(const std::string& exp_name) const {
    std::shared_lock lock(mtx_);
    auto it = experiments_.find(exp_name);
    if (it == experiments_.end()) return Json::Value();

    const auto& exp = it->second;
    Json::Value out;
    out["name"] = exp.name;
    out["trigger_model"] = exp.trigger_model;
    out["enabled"] = exp.enabled;
    out["created_at"] = static_cast<Json::Int64>(exp.created_at);

    Json::Value variants(Json::arrayValue);
    for (const auto& var : exp.variants) {
        Json::Value v;
        v["model"] = var.model;
        v["provider"] = var.provider;
        v["weight"] = var.weight;

        auto mit = exp.metrics.find(var.model);
        if (mit != exp.metrics.end()) {
            const auto& m = mit->second;
            Json::Value metrics;
            metrics["request_count"] = static_cast<Json::Int64>(m.request_count);
            metrics["total_tokens"] = static_cast<Json::Int64>(m.total_tokens);
            metrics["error_count"] = static_cast<Json::Int64>(m.error_count);
            metrics["avg_latency_ms"] = m.request_count > 0
                ? m.total_latency_ms / m.request_count : 0.0;
            metrics["p50_latency_ms"] = compute_percentile(m.latencies, 0.50);
            metrics["p95_latency_ms"] = compute_percentile(m.latencies, 0.95);
            metrics["p99_latency_ms"] = compute_percentile(m.latencies, 0.99);
            metrics["error_rate"] = m.request_count > 0
                ? static_cast<double>(m.error_count) / m.request_count : 0.0;
            v["metrics"] = metrics;
        }
        variants.append(v);
    }
    out["variants"] = variants;
    return out;
}

Json::Value ABTestingPlugin::list_experiments() const {
    std::shared_lock lock(mtx_);
    Json::Value out(Json::arrayValue);
    for (const auto& [name, exp] : experiments_) {
        Json::Value e;
        e["name"] = exp.name;
        e["trigger_model"] = exp.trigger_model;
        e["enabled"] = exp.enabled;
        e["variants_count"] = static_cast<int>(exp.variants.size());

        int64_t total_requests = 0;
        for (const auto& [_, m] : exp.metrics)
            total_requests += m.request_count;
        e["total_requests"] = static_cast<Json::Int64>(total_requests);
        out.append(e);
    }
    return out;
}

bool ABTestingPlugin::add_experiment(const ABExperiment& exp) {
    if (exp.variants.size() < 2 || exp.variants.size() > 5) return false;
    int total_weight = 0;
    for (const auto& v : exp.variants) total_weight += v.weight;
    if (total_weight != 100) return false;

    std::unique_lock lock(mtx_);
    ABExperiment copy = exp;
    copy.created_at = crypto::epoch_seconds();
    for (const auto& v : copy.variants) {
        ABVariantMetrics m;
        m.model = v.model;
        copy.metrics[v.model] = m;
    }
    experiments_[copy.name] = std::move(copy);
    return true;
}

bool ABTestingPlugin::remove_experiment(const std::string& exp_name) {
    std::unique_lock lock(mtx_);
    return experiments_.erase(exp_name) > 0;
}
