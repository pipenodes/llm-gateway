#include "guardrails_plugin.h"
#include "../../tenant_ctx.h"
#include "../logger.h"
#include "../crypto.h"
#include <algorithm>
#include <sstream>
#include <random>

// ── Helpers ──────────────────────────────────────────────────────────────────

static void load_l1(GuardrailsL1Policy& l1, const Json::Value& cfg) {
    l1.max_payload_bytes = static_cast<size_t>(cfg.get("max_payload_bytes", 1048576).asInt64());
    l1.requests_per_minute = cfg.get("requests_per_minute", 60.0).asDouble();
    l1.burst               = cfg.get("burst", 10.0).asDouble();
    l1.block_threshold     = cfg.get("block_threshold", 0.7).asDouble();

    if (cfg.isMember("allow_models") && cfg["allow_models"].isArray())
        for (const auto& m : cfg["allow_models"]) l1.allow_models.push_back(m.asString());

    if (cfg.isMember("deny_models") && cfg["deny_models"].isArray())
        for (const auto& m : cfg["deny_models"]) l1.deny_models.push_back(m.asString());

    if (cfg.isMember("trusted_client_ids") && cfg["trusted_client_ids"].isArray())
        for (const auto& c : cfg["trusted_client_ids"]) l1.trusted_client_ids.push_back(c.asString());
}

static void load_l2(GuardrailsL2Policy& l2, const Json::Value& cfg) {
    l2.enabled              = cfg.get("enabled", false).asBool();
    l2.sample_rate          = cfg.get("sample_rate", 0.5).asDouble();
    l2.toxicity_threshold   = cfg.get("toxicity_threshold", 0.8).asDouble();
    l2.jailbreak_threshold  = cfg.get("jailbreak_threshold", 0.7).asDouble();
}

static void load_l3(GuardrailsL3Policy& l3, const Json::Value& cfg) {
    l3.enabled     = cfg.get("enabled", false).asBool();
    l3.sample_rate = cfg.get("sample_rate", 0.05).asDouble();
    l3.judge_model = cfg.get("judge_model", "llama3:8b").asString();
}

static GuardrailsTenantPolicy load_policy(const Json::Value& cfg) {
    GuardrailsTenantPolicy p;
    if (cfg.isMember("l1") && cfg["l1"].isObject()) load_l1(p.l1, cfg["l1"]);
    if (cfg.isMember("l2") && cfg["l2"].isObject()) load_l2(p.l2, cfg["l2"]);
    if (cfg.isMember("l3") && cfg["l3"].isObject()) load_l3(p.l3, cfg["l3"]);
    return p;
}

// ── Init ─────────────────────────────────────────────────────────────────────

bool GuardrailsPlugin::init(const Json::Value& config) {
    if (config.isMember("default_policy") && config["default_policy"].isObject())
        default_policy_ = load_policy(config["default_policy"]);

    if (config.isMember("tenant_policies") && config["tenant_policies"].isObject()) {
        for (const auto& key : config["tenant_policies"].getMemberNames())
            tenant_policies_[key] = load_policy(config["tenant_policies"][key]);
    }

    // Built-in prompt injection / PII patterns for L1 regex checks
    auto add = [this](const std::string& name, const std::string& pat, float weight) {
        try {
            injection_patterns_.push_back({name, std::regex(pat, std::regex::icase), weight});
        } catch (const std::regex_error&) {}
    };
    add("ignore_instructions",  R"(ignore\s+(previous|all|above|prior)\s+instructions?)", 0.4f);
    add("jailbreak_prefix",     R"(\bdana\b|\bDAN\b|jailbreak|act\s+as\s+if\s+you\s+are)", 0.35f);
    add("system_override",      R"(system\s*prompt|you\s+are\s+now|forget\s+your\s+training)", 0.3f);
    add("role_injection",       R"(<\|?(system|assistant|user)\|?>)", 0.25f);

    Json::Value f;
    f["tenant_policies"] = static_cast<int>(tenant_policies_.size());
    f["l1_patterns"]     = static_cast<int>(injection_patterns_.size());
    Logger::info("guardrails_init", f);
    return true;
}

// ── Policy resolution ─────────────────────────────────────────────────────────

const GuardrailsTenantPolicy& GuardrailsPlugin::get_policy(const std::string& tenant,
                                                            const std::string& app) const {
    auto key = tenant + ":" + app;
    auto it = tenant_policies_.find(key);
    if (it != tenant_policies_.end()) return it->second;
    // fallback: tenant-only
    it = tenant_policies_.find(tenant);
    if (it != tenant_policies_.end()) return it->second;
    return default_policy_;
}

// ── Token bucket rate limiter ─────────────────────────────────────────────────

bool GuardrailsPlugin::consume_token(const std::string& rate_key,
                                      const GuardrailsL1Policy& pol) {
    std::unique_lock lock(rate_mtx_);
    int64_t now = crypto::epoch_seconds();
    auto& b = rate_buckets_[rate_key];
    if (b.capacity == 0.0) {
        b.capacity        = pol.burst;
        b.refill_per_sec  = pol.requests_per_minute / 60.0;
        b.tokens          = b.capacity;
        b.last_refill_epoch = now;
    }
    double elapsed = static_cast<double>(now - b.last_refill_epoch);
    b.tokens = std::min(b.capacity, b.tokens + elapsed * b.refill_per_sec);
    b.last_refill_epoch = now;
    if (b.tokens >= 1.0) {
        b.tokens -= 1.0;
        return true;
    }
    return false;
}

// ── Text extraction ───────────────────────────────────────────────────────────

std::string GuardrailsPlugin::extract_text(const Json::Value& body) const {
    std::ostringstream oss;
    if (body.isMember("messages") && body["messages"].isArray()) {
        for (const auto& msg : body["messages"])
            if (msg.isMember("content") && msg["content"].isString())
                oss << msg["content"].asString() << ' ';
    } else if (body.isMember("prompt") && body["prompt"].isString()) {
        oss << body["prompt"].asString();
    }
    return oss.str();
}

// ── Regex scoring ─────────────────────────────────────────────────────────────

float GuardrailsPlugin::score_prompt(const std::string& text) const {
    float score = 0.0f;
    for (const auto& pat : injection_patterns_) {
        if (std::regex_search(text, pat.pattern))
            score += pat.weight;
    }
    return std::min(score, 1.0f);
}

// ── L1 check ─────────────────────────────────────────────────────────────────

PluginResult GuardrailsPlugin::check_l1(const Json::Value& body,
                                         PluginRequestContext& ctx,
                                         const GuardrailsTenantPolicy& policy) {
    const auto& l1 = policy.l1;

    // 1. Payload size
    std::string body_str = body.toStyledString();
    if (body_str.size() > l1.max_payload_bytes) {
        Json::Value f;
        f["tenant_id"] = ctx_tenant(ctx);
        f["client_id"] = ctx_client(ctx);
        f["size"]      = static_cast<Json::Int64>(body_str.size());
        f["limit"]     = static_cast<Json::Int64>(l1.max_payload_bytes);
        Logger::warn("guardrails_l1_payload_too_large", f);
        l1_blocked_.fetch_add(1, std::memory_order_relaxed);
        return PluginResult{.action = PluginAction::Block, .error_code = 413,
            .error_message = "Request payload exceeds limit"};
    }

    // 2. Model allow/deny lists
    const std::string& model = ctx.model;
    if (!l1.allow_models.empty()) {
        bool allowed = std::find(l1.allow_models.begin(), l1.allow_models.end(), model)
                       != l1.allow_models.end();
        if (!allowed) {
            l1_blocked_.fetch_add(1, std::memory_order_relaxed);
            return PluginResult{.action = PluginAction::Block, .error_code = 403,
                .error_message = "Model not in allow list for this tenant"};
        }
    }
    if (!l1.deny_models.empty()) {
        bool denied = std::find(l1.deny_models.begin(), l1.deny_models.end(), model)
                      != l1.deny_models.end();
        if (denied) {
            l1_blocked_.fetch_add(1, std::memory_order_relaxed);
            return PluginResult{.action = PluginAction::Block, .error_code = 403,
                .error_message = "Model is blocked for this tenant"};
        }
    }

    // 3. Per-tenant:client_id rate limit (skip for trusted clients)
    std::string client = ctx_client(ctx);
    bool trusted = std::find(l1.trusted_client_ids.begin(), l1.trusted_client_ids.end(), client)
                   != l1.trusted_client_ids.end();
    if (!trusted) {
        std::string rate_key = ctx_rate_key(ctx);
        if (!consume_token(rate_key, l1)) {
            l1_blocked_.fetch_add(1, std::memory_order_relaxed);
            return PluginResult{.action = PluginAction::Block, .error_code = 429,
                .error_message = "Rate limit exceeded for tenant"};
        }
    }

    // 4. Regex patterns (prompt injection / PII)
    std::string text = extract_text(body);
    if (!text.empty()) {
        float score = score_prompt(text);
        if (score >= static_cast<float>(l1.block_threshold)) {
            Json::Value f;
            f["tenant_id"] = ctx_tenant(ctx);
            f["score"]     = score;
            f["threshold"] = l1.block_threshold;
            Logger::warn("guardrails_l1_injection_blocked", f);
            l1_blocked_.fetch_add(1, std::memory_order_relaxed);
            return PluginResult{.action = PluginAction::Block, .error_code = 400,
                .error_message = "Request blocked by security policy"};
        }
        // Store score for observability (downstream plugins/logs)
        ctx.metadata["guardrails_l1_score"] = std::to_string(score);
    }

    return PluginResult{.action = PluginAction::Continue};
}

// ── before_request ────────────────────────────────────────────────────────────

PluginResult GuardrailsPlugin::before_request(Json::Value& body, PluginRequestContext& ctx) {
    total_requests_.fetch_add(1, std::memory_order_relaxed);

    const auto& policy = get_policy(ctx_tenant(ctx), ctx_app(ctx));

    // L1 — synchronous, always executed
    PluginResult l1_result = check_l1(body, ctx, policy);
    if (l1_result.action == PluginAction::Block) return l1_result;

    // L2 — ML classifier stub (ONNX inference would be wired here)
    if (policy.l2.enabled) {
        thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        if (dist(rng) < policy.l2.sample_rate) {
            // Placeholder: actual ONNX inference goes here
            // float toxicity = onnx_infer("toxicity", extract_text(body));
            // float jailbreak = onnx_infer("jailbreak", extract_text(body));
            // if (toxicity > policy.l2.toxicity_threshold || jailbreak > policy.l2.jailbreak_threshold)
            //     return Block(400, "Content blocked by ML classifier");
            ctx.metadata["guardrails_l2_sampled"] = "1";
        }
    }

    // L3 — async LLM Judge (never blocks the client response)
    if (policy.l3.enabled) {
        thread_local std::mt19937 rng2(std::random_device{}());
        std::uniform_real_distribution<double> dist2(0.0, 1.0);
        if (dist2(rng2) < policy.l3.sample_rate) {
            // Fire-and-forget: actual LLM judge call would go here in a detached thread
            // std::thread([...] { call_llm_judge(judge_model, text); }).detach();
            ctx.metadata["guardrails_l3_sampled"] = "1";
        }
    }

    return PluginResult{.action = PluginAction::Continue};
}

// ── after_response ────────────────────────────────────────────────────────────

PluginResult GuardrailsPlugin::after_response(Json::Value& /*response*/,
                                               PluginRequestContext& /*ctx*/) {
    return PluginResult{.action = PluginAction::Continue};
}

// ── stats ─────────────────────────────────────────────────────────────────────

Json::Value GuardrailsPlugin::stats() const {
    Json::Value out;
    out["total_requests"]    = static_cast<Json::Int64>(total_requests_.load());
    out["l1_blocked"]        = static_cast<Json::Int64>(l1_blocked_.load());
    out["l2_blocked"]        = static_cast<Json::Int64>(l2_blocked_.load());
    out["tenant_policies"]   = static_cast<int>(tenant_policies_.size());
    {
        std::shared_lock lock(rate_mtx_);
        out["active_rate_buckets"] = static_cast<int>(rate_buckets_.size());
    }
    return out;
}
