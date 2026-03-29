#include "data_discovery_plugin.h"
#include "../../tenant_ctx.h"
#include "../logger.h"
#include "../crypto.h"
#include <fstream>
#include <sstream>
#include <functional>
#include <cstdio>

// ── Hash (deterministic, no raw content stored) ───────────────────────────────

std::string DataDiscoveryPlugin::compute_hash(const std::string& data) const {
    std::hash<std::string> h;
    size_t h1 = h(data);
    size_t h2 = h(data + "\xca\xfe\xba\xbe");
    char buf[33];
    std::snprintf(buf, sizeof(buf), "%016zx%016zx", h1, h2);
    return buf;
}

// ── Rule loading ──────────────────────────────────────────────────────────────

void DataDiscoveryPlugin::load_rules(const Json::Value& rules_cfg,
                                      std::vector<DiscoveryRule>& out) {
    if (!rules_cfg.isArray()) return;
    for (const auto& r : rules_cfg) {
        std::string pat = r.get("pattern", "").asString();
        std::string tag = r.get("tags", Json::Value(Json::arrayValue)).isArray()
                          ? (r["tags"].size() > 0 ? r["tags"][0].asString() : "confidential")
                          : r.get("tags", "confidential").asString();
        if (pat.empty()) continue;
        try {
            out.push_back({r.get("kind", "regex").asString(), std::regex(pat), tag});
        } catch (const std::regex_error&) {}
    }
}

// ── Init ──────────────────────────────────────────────────────────────────────

bool DataDiscoveryPlugin::init(const Json::Value& config) {
    catalog_path_           = config.get("catalog_path", "data/discovery-catalog.json").asString();
    flush_interval_seconds_ = config.get("flush_interval_seconds", 300).asInt();

    load_rules(config["global_rules"], global_rules_);

    if (config.isMember("tenant_rules") && config["tenant_rules"].isObject()) {
        for (const auto& key : config["tenant_rules"].getMemberNames()) {
            std::vector<DiscoveryRule> rules;
            load_rules(config["tenant_rules"][key], rules);
            tenant_rules_[key] = std::move(rules);
        }
    }

    if (config.isMember("shadow_ai") && config["shadow_ai"].isObject()) {
        shadow_ai_enabled_ = config["shadow_ai"].get("enabled", true).asBool();

        auto load_set = [](const Json::Value& node, const std::string& key,
                           std::unordered_map<std::string,
                               std::unordered_set<std::string>>& out) {
            if (node.isMember(key) && node[key].isObject()) {
                for (const auto& tenant : node[key].getMemberNames()) {
                    for (const auto& v : node[key][tenant])
                        out[tenant].insert(v.asString());
                }
            }
        };
        load_set(config["shadow_ai"], "allowed_models_by_tenant",    allowed_models_);
        load_set(config["shadow_ai"], "allowed_endpoints_by_tenant",  allowed_endpoints_);
    }

    load();

    running_ = true;
    flush_thread_ = std::jthread([this] { flush_loop(); });

    Json::Value f;
    f["global_rules"]  = static_cast<int>(global_rules_.size());
    f["tenant_rules"]  = static_cast<int>(tenant_rules_.size());
    f["shadow_ai"]     = shadow_ai_enabled_;
    Logger::info("data_discovery_init", f);
    return true;
}

void DataDiscoveryPlugin::shutdown() {
    if (running_.exchange(false)) {
        if (flush_thread_.joinable()) flush_thread_.join();
        std::shared_lock lock(catalog_mtx_);
        save_unlocked();
    }
}

// ── Text extraction ───────────────────────────────────────────────────────────

std::string DataDiscoveryPlugin::extract_text(const Json::Value& body) const {
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

// ── Classification ────────────────────────────────────────────────────────────

void DataDiscoveryPlugin::classify_text(const std::string& text,
                                         const std::string& tenant,
                                         const std::string& app,
                                         std::unordered_set<std::string>& tags_out) {
    auto run_rules = [&](const std::vector<DiscoveryRule>& rules) {
        for (const auto& rule : rules) {
            std::sregex_iterator it(text.begin(), text.end(), rule.pattern), end;
            while (it != end) {
                tags_out.insert(rule.tag);
                std::string match_str = (*it)[0].str();
                std::string hash = compute_hash(match_str);
                record_catalog(hash, rule.tag, tenant, app);
                ++it;
            }
        }
    };

    run_rules(global_rules_);

    // Tenant-specific rules (fallback to tenant-only key if no app-scoped rules)
    auto tkey = tenant + ":" + app;
    if (tenant_rules_.count(tkey)) run_rules(tenant_rules_.at(tkey));
    else if (tenant_rules_.count(tenant)) run_rules(tenant_rules_.at(tenant));
}

// ── Catalog recording ─────────────────────────────────────────────────────────

void DataDiscoveryPlugin::record_catalog(const std::string& hash, const std::string& tag,
                                          const std::string& tenant, const std::string& app) {
    int64_t now = crypto::epoch_seconds();
    std::unique_lock lock(catalog_mtx_);
    auto& entry = catalog_[hash];
    if (entry.content_hash.empty()) {
        entry.content_hash   = hash;
        entry.tag            = tag;
        entry.tenant_id      = tenant;
        entry.app_id         = app;
        entry.first_seen_epoch = now;
        entry.occurrences    = 0;
    }
    entry.last_seen_epoch = now;
    entry.occurrences++;
    dirty_ = true;
    total_classified_.fetch_add(1, std::memory_order_relaxed);
}

// ── Shadow AI detection ───────────────────────────────────────────────────────

void DataDiscoveryPlugin::check_shadow_ai(const PluginRequestContext& ctx) {
    if (!shadow_ai_enabled_) return;

    std::string tenant = ctx_tenant(ctx);
    const std::string& model = ctx.model;

    auto it = allowed_models_.find(tenant);
    bool model_allowed = (it == allowed_models_.end()) || it->second.count(model) > 0;

    if (!model_allowed) {
        ShadowAIEvent ev;
        ev.tenant_id    = tenant;
        ev.app_id       = ctx_app(ctx);
        ev.model        = model;
        ev.endpoint     = ctx.path;
        ev.detected_at  = crypto::epoch_seconds();

        {
            std::unique_lock lock(catalog_mtx_);
            shadow_ai_events_.push_back(ev);
            dirty_ = true;
        }

        shadow_ai_detections_.fetch_add(1, std::memory_order_relaxed);

        Json::Value f;
        f["tenant_id"] = tenant;
        f["model"]     = model;
        Logger::warn("shadow_ai_detected", f);
    }
}

// ── before_request ────────────────────────────────────────────────────────────

PluginResult DataDiscoveryPlugin::before_request(Json::Value& body,
                                                  PluginRequestContext& ctx) {
    std::string text   = extract_text(body);
    std::string tenant = ctx_tenant(ctx);
    std::string app    = ctx_app(ctx);

    std::unordered_set<std::string> tags;
    if (!text.empty())
        classify_text(text, tenant, app, tags);

    // Shadow AI: check if model is inventoried for this tenant
    check_shadow_ai(ctx);

    // Propagate tags for DLP plugin downstream
    if (!tags.empty()) {
        std::ostringstream oss;
        bool first = true;
        for (const auto& t : tags) {
            if (!first) oss << ',';
            oss << t;
            first = false;
        }
        ctx.metadata[DISCOVERY_TAGS_KEY] = oss.str();
    }

    return PluginResult{.action = PluginAction::Continue};
}

// ── after_response (classify response content as well) ────────────────────────

PluginResult DataDiscoveryPlugin::after_response(Json::Value& response,
                                                  PluginRequestContext& ctx) {
    std::string tenant = ctx_tenant(ctx);
    std::string app    = ctx_app(ctx);

    std::string text;
    if (response.isMember("choices") && response["choices"].isArray()
        && !response["choices"].empty()) {
        const auto& choice = response["choices"][0];
        if (choice.isMember("message") && choice["message"].isMember("content")
            && choice["message"]["content"].isString())
            text = choice["message"]["content"].asString();
    }

    if (!text.empty()) {
        std::unordered_set<std::string> tags;
        classify_text(text, tenant, app, tags);
        // Merge response tags into existing discovered_tags
        if (!tags.empty()) {
            auto existing = ctx.metadata.find(DISCOVERY_TAGS_KEY);
            std::ostringstream oss;
            if (existing != ctx.metadata.end() && !existing->second.empty())
                oss << existing->second << ',';
            bool first = true;
            for (const auto& t : tags) {
                if (!first) oss << ',';
                oss << t;
                first = false;
            }
            ctx.metadata[DISCOVERY_TAGS_KEY] = oss.str();
        }
    }

    return PluginResult{.action = PluginAction::Continue};
}

// ── Admin queries ─────────────────────────────────────────────────────────────

Json::Value DataDiscoveryPlugin::get_catalog(const std::string& tenant_id) const {
    std::shared_lock lock(catalog_mtx_);
    Json::Value out(Json::arrayValue);
    for (const auto& [hash, entry] : catalog_) {
        if (!tenant_id.empty() && entry.tenant_id != tenant_id) continue;
        Json::Value row;
        row["hash"]       = entry.content_hash;
        row["tag"]        = entry.tag;
        row["tenant_id"]  = entry.tenant_id;
        row["app_id"]     = entry.app_id;
        row["occurrences"]= static_cast<Json::Int64>(entry.occurrences);
        row["first_seen"] = static_cast<Json::Int64>(entry.first_seen_epoch);
        row["last_seen"]  = static_cast<Json::Int64>(entry.last_seen_epoch);
        out.append(row);
    }
    return out;
}

Json::Value DataDiscoveryPlugin::get_shadow_ai(const std::string& tenant_id) const {
    std::shared_lock lock(catalog_mtx_);
    Json::Value out(Json::arrayValue);
    for (const auto& ev : shadow_ai_events_) {
        if (!tenant_id.empty() && ev.tenant_id != tenant_id) continue;
        Json::Value row;
        row["tenant_id"]   = ev.tenant_id;
        row["app_id"]      = ev.app_id;
        row["model"]       = ev.model;
        row["endpoint"]    = ev.endpoint;
        row["detected_at"] = static_cast<Json::Int64>(ev.detected_at);
        out.append(row);
    }
    return out;
}

// ── Stats ─────────────────────────────────────────────────────────────────────

Json::Value DataDiscoveryPlugin::stats() const {
    Json::Value out;
    out["total_classified"]    = static_cast<Json::Int64>(total_classified_.load());
    out["shadow_ai_detections"]= static_cast<Json::Int64>(shadow_ai_detections_.load());
    {
        std::shared_lock lock(catalog_mtx_);
        out["catalog_entries"]  = static_cast<int>(catalog_.size());
        out["shadow_ai_events"] = static_cast<int>(shadow_ai_events_.size());
    }
    out["global_rules"] = static_cast<int>(global_rules_.size());
    return out;
}

// ── Persistence ───────────────────────────────────────────────────────────────

void DataDiscoveryPlugin::load() {
    if (catalog_path_.empty()) return;
    std::ifstream file(catalog_path_);
    if (!file.is_open()) return;

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    if (!Json::parseFromStream(builder, file, &root, &errs) || !root.isObject()) return;

    std::unique_lock lock(catalog_mtx_);
    if (root.isMember("catalog") && root["catalog"].isArray()) {
        for (const auto& row : root["catalog"]) {
            CatalogEntry e;
            e.content_hash      = row.get("hash", "").asString();
            e.tag               = row.get("tag", "").asString();
            e.tenant_id         = row.get("tenant_id", "").asString();
            e.app_id            = row.get("app_id", "").asString();
            e.first_seen_epoch  = row.get("first_seen", 0).asInt64();
            e.last_seen_epoch   = row.get("last_seen", 0).asInt64();
            e.occurrences       = row.get("occurrences", 0).asInt64();
            if (!e.content_hash.empty()) catalog_[e.content_hash] = e;
        }
    }
    if (root.isMember("shadow_ai") && root["shadow_ai"].isArray()) {
        for (const auto& row : root["shadow_ai"]) {
            ShadowAIEvent ev;
            ev.tenant_id   = row.get("tenant_id", "").asString();
            ev.app_id      = row.get("app_id", "").asString();
            ev.model       = row.get("model", "").asString();
            ev.endpoint    = row.get("endpoint", "").asString();
            ev.detected_at = row.get("detected_at", 0).asInt64();
            shadow_ai_events_.push_back(ev);
        }
    }
}

void DataDiscoveryPlugin::save_unlocked() const {
    if (catalog_path_.empty()) return;
    Json::Value root(Json::objectValue);
    Json::Value catalog_arr(Json::arrayValue);
    for (const auto& [hash, e] : catalog_) {
        Json::Value row;
        row["hash"]       = e.content_hash;
        row["tag"]        = e.tag;
        row["tenant_id"]  = e.tenant_id;
        row["app_id"]     = e.app_id;
        row["first_seen"] = static_cast<Json::Int64>(e.first_seen_epoch);
        row["last_seen"]  = static_cast<Json::Int64>(e.last_seen_epoch);
        row["occurrences"]= static_cast<Json::Int64>(e.occurrences);
        catalog_arr.append(row);
    }
    root["catalog"] = catalog_arr;

    Json::Value shadow_arr(Json::arrayValue);
    for (const auto& ev : shadow_ai_events_) {
        Json::Value row;
        row["tenant_id"]   = ev.tenant_id;
        row["app_id"]      = ev.app_id;
        row["model"]       = ev.model;
        row["endpoint"]    = ev.endpoint;
        row["detected_at"] = static_cast<Json::Int64>(ev.detected_at);
        shadow_arr.append(row);
    }
    root["shadow_ai"] = shadow_arr;

    Json::StreamWriterBuilder w;
    w["indentation"] = "  ";
    std::ofstream file(catalog_path_);
    if (file.is_open()) file << Json::writeString(w, root) << '\n';
}

void DataDiscoveryPlugin::flush_loop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(flush_interval_seconds_));
        if (!running_) break;
        if (dirty_.exchange(false)) {
            std::shared_lock lock(catalog_mtx_);
            save_unlocked();
        }
    }
}
