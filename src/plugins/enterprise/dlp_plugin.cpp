#include "dlp_plugin.h"
#include "data_discovery_plugin.h"  // for DISCOVERY_TAGS_KEY
#include "../../tenant_ctx.h"
#include "../logger.h"
#include "../crypto.h"
#include <sstream>
#include <filesystem>

// ── Policy loading ────────────────────────────────────────────────────────────

static DLPAction action_from_string(const std::string& s) {
    if (s == "block")      return DLPAction::Block;
    if (s == "redact")     return DLPAction::Redact;
    if (s == "quarantine") return DLPAction::Quarantine;
    if (s == "allow")      return DLPAction::Allow;
    return DLPAction::Alert;
}

DLPTenantPolicy DLPPlugin::load_policy(const Json::Value& cfg) const {
    DLPTenantPolicy p;
    p.log_detections = cfg.get("log_detections", true).asBool();
    p.audit_trail    = cfg.get("audit_trail", true).asBool();

    if (cfg.isMember("type_policies") && cfg["type_policies"].isArray()) {
        for (const auto& tp : cfg["type_policies"]) {
            DLPTypePolicy policy;
            policy.data_type            = tp.get("data_type", "pii").asString();
            policy.action               = action_from_string(tp.get("action", "alert").asString());
            policy.confidence_threshold = tp.get("confidence_threshold", 0.8).asDouble();
            p.type_policies.push_back(policy);
        }
    }
    return p;
}

bool DLPPlugin::init(const Json::Value& config) {
    audit_path_      = config.get("audit_path", "data/dlp-audit").asString();
    quarantine_path_ = config.get("quarantine_path", "data/dlp-quarantine").asString();

    // Ensure directories exist
    try {
        std::filesystem::create_directories(audit_path_);
        std::filesystem::create_directories(quarantine_path_);
    } catch (...) {}

    if (config.isMember("default_policy") && config["default_policy"].isObject())
        default_policy_ = load_policy(config["default_policy"]);

    if (config.isMember("tenant_policies") && config["tenant_policies"].isObject()) {
        for (const auto& key : config["tenant_policies"].getMemberNames())
            tenant_policies_[key] = load_policy(config["tenant_policies"][key]);
    }

    Json::Value f;
    f["tenant_policies"] = static_cast<int>(tenant_policies_.size());
    f["audit_path"]      = audit_path_;
    Logger::info("dlp_init", f);
    return true;
}

void DLPPlugin::shutdown() {}

// ── Policy resolution ─────────────────────────────────────────────────────────

const DLPTenantPolicy& DLPPlugin::get_policy(const std::string& tenant,
                                               const std::string& app) const {
    auto key = tenant + ":" + app;
    auto it = tenant_policies_.find(key);
    if (it != tenant_policies_.end()) return it->second;
    it = tenant_policies_.find(tenant);
    if (it != tenant_policies_.end()) return it->second;
    return default_policy_;
}

DLPAction DLPPlugin::resolve_action(const DLPTenantPolicy& policy,
                                     const std::string& tag) const {
    for (const auto& tp : policy.type_policies) {
        if (tp.data_type == tag) return tp.action;
    }
    return DLPAction::Alert; // default: log but don't block
}

// ── Audit trail ───────────────────────────────────────────────────────────────

void DLPPlugin::write_audit(const PluginRequestContext& ctx, const std::string& tag,
                             DLPAction action, const std::string& source) const {
    std::string action_str;
    switch (action) {
        case DLPAction::Block:      action_str = "block";      break;
        case DLPAction::Redact:     action_str = "redact";     break;
        case DLPAction::Quarantine: action_str = "quarantine"; break;
        case DLPAction::Alert:      action_str = "alert";      break;
        default:                    action_str = "allow";      break;
    }

    Json::Value entry;
    entry["ts"]         = static_cast<Json::Int64>(crypto::epoch_seconds());
    entry["request_id"] = ctx.request_id;
    entry["tenant_id"]  = ctx_tenant(ctx);
    entry["app_id"]     = ctx_app(ctx);
    entry["client_id"]  = ctx_client(ctx);
    entry["tag"]        = tag;
    entry["action"]     = action_str;
    entry["source"]     = source; // "request" or "response"

    thread_local Json::StreamWriterBuilder w = [] {
        Json::StreamWriterBuilder b; b["indentation"] = ""; return b;
    }();

    std::string line = Json::writeString(w, entry) + "\n";
    std::string path = audit_path_ + "/dlp-" + ctx_tenant(ctx) + ".jsonl";

    std::lock_guard lock(audit_mtx_);
    std::ofstream file(path, std::ios::app);
    if (file.is_open()) file << line;
}

// ── Quarantine ────────────────────────────────────────────────────────────────

void DLPPlugin::add_quarantine(const PluginRequestContext& ctx,
                                const std::string& tag, const std::string& reason) {
    QuarantineEntry qe;
    qe.request_id = ctx.request_id;
    qe.tenant_id  = ctx_tenant(ctx);
    qe.app_id     = ctx_app(ctx);
    qe.client_id  = ctx_client(ctx);
    qe.tag        = tag;
    qe.timestamp  = crypto::epoch_seconds();
    qe.reason     = reason;

    {
        std::unique_lock lock(quarantine_mtx_);
        quarantine_.push_back(qe);
    }

    // Also persist to JSONL
    Json::Value entry;
    entry["ts"]         = static_cast<Json::Int64>(qe.timestamp);
    entry["request_id"] = qe.request_id;
    entry["tenant_id"]  = qe.tenant_id;
    entry["app_id"]     = qe.app_id;
    entry["client_id"]  = qe.client_id;
    entry["tag"]        = qe.tag;
    entry["reason"]     = qe.reason;

    thread_local Json::StreamWriterBuilder w = [] {
        Json::StreamWriterBuilder b; b["indentation"] = ""; return b;
    }();

    std::string path = quarantine_path_ + "/quarantine-" + qe.tenant_id + ".jsonl";
    std::lock_guard lock(audit_mtx_);
    std::ofstream file(path, std::ios::app);
    if (file.is_open()) file << Json::writeString(w, entry) << '\n';
}

// ── Redaction (simple placeholder replacement) ────────────────────────────────

std::string DLPPlugin::redact_body(Json::Value& body) const {
    if (!body.isMember("messages") || !body["messages"].isArray()) return "";
    int count = 0;
    for (auto& msg : body["messages"]) {
        if (msg.isMember("content") && msg["content"].isString()) {
            std::string& content = const_cast<std::string&>(msg["content"].asStringRef());
            // Simple redaction: mark as [REDACTED-BY-DLP]
            msg["content"] = "[REDACTED-BY-DLP]";
            count++;
        }
    }
    return std::to_string(count);
}

// ── Inspect + apply policy ────────────────────────────────────────────────────

PluginResult DLPPlugin::apply_dlp(DLPPlugin* self, const std::vector<std::string>& tags,
                                   const DLPTenantPolicy& policy, Json::Value& body,
                                   PluginRequestContext& ctx, const std::string& source) {
    // Determine the most restrictive action across all detected tags
    DLPAction worst = DLPAction::Allow;
    std::string worst_tag;
    for (const auto& tag : tags) {
        DLPAction a = self->get_policy(ctx_tenant(ctx), ctx_app(ctx)).type_policies.empty()
                      ? DLPAction::Alert
                      : [&]() {
                          for (const auto& tp : policy.type_policies)
                              if (tp.data_type == tag) return tp.action;
                          return DLPAction::Alert;
                      }();
        // Action severity: Quarantine > Block > Redact > Alert > Allow
        auto severity = [](DLPAction x) -> int {
            switch (x) {
                case DLPAction::Quarantine: return 4;
                case DLPAction::Block:      return 3;
                case DLPAction::Redact:     return 2;
                case DLPAction::Alert:      return 1;
                default:                    return 0;
            }
        };
        if (severity(a) > severity(worst)) { worst = a; worst_tag = tag; }
    }

    if (policy.log_detections && worst != DLPAction::Allow) {
        Json::Value f;
        f["tenant_id"] = ctx_tenant(ctx);
        f["tag"]       = worst_tag;
        f["source"]    = source;
        Logger::warn("dlp_detection", f);
    }

    if (policy.audit_trail && worst != DLPAction::Allow)
        self->write_audit(ctx, worst_tag, worst, source);

    switch (worst) {
        case DLPAction::Block:
            self->blocked_.fetch_add(1, std::memory_order_relaxed);
            return PluginResult{.action = PluginAction::Block, .error_code = 403,
                .error_message = "Content blocked by data loss prevention policy"};
        case DLPAction::Quarantine:
            self->add_quarantine(ctx, worst_tag, "DLP quarantine: " + source);
            self->quarantined_.fetch_add(1, std::memory_order_relaxed);
            return PluginResult{.action = PluginAction::Block, .error_code = 403,
                .error_message = "Content quarantined by data loss prevention policy"};
        case DLPAction::Redact:
            self->redact_body(body);
            self->redacted_.fetch_add(1, std::memory_order_relaxed);
            return PluginResult{.action = PluginAction::Continue};
        default:
            return PluginResult{.action = PluginAction::Continue};
    }
}

// ── before_request ────────────────────────────────────────────────────────────

PluginResult DLPPlugin::before_request(Json::Value& body, PluginRequestContext& ctx) {
    total_inspected_.fetch_add(1, std::memory_order_relaxed);

    auto tags_it = ctx.metadata.find(DISCOVERY_TAGS_KEY);
    if (tags_it == ctx.metadata.end() || tags_it->second.empty())
        return PluginResult{.action = PluginAction::Continue};

    // Parse comma-separated tags populated by DataDiscoveryPlugin
    std::vector<std::string> tags;
    std::istringstream iss(tags_it->second);
    std::string tag;
    while (std::getline(iss, tag, ','))
        if (!tag.empty()) tags.push_back(tag);

    if (tags.empty()) return PluginResult{.action = PluginAction::Continue};

    const auto& policy = get_policy(ctx_tenant(ctx), ctx_app(ctx));
    return apply_dlp(this, tags, policy, body, ctx, "request");
}

// ── after_response ────────────────────────────────────────────────────────────

PluginResult DLPPlugin::after_response(Json::Value& response, PluginRequestContext& ctx) {
    // Re-check discovered_tags in case data_discovery added response-side tags
    auto tags_it = ctx.metadata.find(DISCOVERY_TAGS_KEY);
    if (tags_it == ctx.metadata.end() || tags_it->second.empty())
        return PluginResult{.action = PluginAction::Continue};

    std::vector<std::string> tags;
    std::istringstream iss(tags_it->second);
    std::string tag;
    while (std::getline(iss, tag, ','))
        if (!tag.empty()) tags.push_back(tag);

    if (tags.empty()) return PluginResult{.action = PluginAction::Continue};

    const auto& policy = get_policy(ctx_tenant(ctx), ctx_app(ctx));
    return apply_dlp(this, tags, policy, response, ctx, "response");
}

// ── Admin queries ─────────────────────────────────────────────────────────────

Json::Value DLPPlugin::get_quarantine(const std::string& tenant_id) const {
    std::shared_lock lock(quarantine_mtx_);
    Json::Value out(Json::arrayValue);
    for (const auto& qe : quarantine_) {
        if (!tenant_id.empty() && qe.tenant_id != tenant_id) continue;
        Json::Value row;
        row["request_id"] = qe.request_id;
        row["tenant_id"]  = qe.tenant_id;
        row["app_id"]     = qe.app_id;
        row["client_id"]  = qe.client_id;
        row["tag"]        = qe.tag;
        row["timestamp"]  = static_cast<Json::Int64>(qe.timestamp);
        row["reason"]     = qe.reason;
        out.append(row);
    }
    return out;
}

// ── Stats ─────────────────────────────────────────────────────────────────────

Json::Value DLPPlugin::stats() const {
    Json::Value out;
    out["total_inspected"] = static_cast<Json::Int64>(total_inspected_.load());
    out["blocked"]         = static_cast<Json::Int64>(blocked_.load());
    out["redacted"]        = static_cast<Json::Int64>(redacted_.load());
    out["quarantined"]     = static_cast<Json::Int64>(quarantined_.load());
    out["tenant_policies"] = static_cast<int>(tenant_policies_.size());
    return out;
}
