#pragma once
#include "../plugin.h"
#include <regex>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>
#include <atomic>
#include <thread>

// RF-34-FUNCIONAL.md — Data Discovery: classificação contínua de dados em trânsito

static constexpr const char* DISCOVERY_TAGS_KEY = "discovered_tags";

struct DiscoveryRule {
    std::string name;
    std::regex  pattern;
    std::string tag; // "pii", "phi", "pci", "confidential"
};

struct CatalogEntry {
    std::string content_hash;   // SHA-256-like hash (sem conteúdo original)
    std::string tag;
    std::string tenant_id;
    std::string app_id;
    int64_t     first_seen_epoch = 0;
    int64_t     last_seen_epoch  = 0;
    int64_t     occurrences      = 0;
};

struct ShadowAIEvent {
    std::string tenant_id;
    std::string app_id;
    std::string model;
    std::string endpoint;
    int64_t     detected_at = 0;
};

class DataDiscoveryPlugin : public Plugin {
public:
    std::string name() const override { return "data_discovery"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;
    void shutdown() override;
    Json::Value stats() const override;

    Json::Value get_catalog(const std::string& tenant_id) const;
    Json::Value get_shadow_ai(const std::string& tenant_id) const;

private:
    void load_rules(const Json::Value& rules_cfg, std::vector<DiscoveryRule>& out);
    std::string compute_hash(const std::string& data) const;
    std::string extract_text(const Json::Value& body) const;
    void classify_text(const std::string& text, const std::string& tenant,
                       const std::string& app, std::unordered_set<std::string>& tags_out);
    void record_catalog(const std::string& hash, const std::string& tag,
                        const std::string& tenant, const std::string& app);
    void check_shadow_ai(const PluginRequestContext& ctx);
    void load();
    void save_unlocked() const;
    void flush_loop();

    std::vector<DiscoveryRule> global_rules_;
    std::unordered_map<std::string, std::vector<DiscoveryRule>> tenant_rules_; // "tenant:app"

    // Shadow AI config: tenant → allowed models/endpoints
    std::unordered_map<std::string, std::unordered_set<std::string>> allowed_models_;
    std::unordered_map<std::string, std::unordered_set<std::string>> allowed_endpoints_;
    bool shadow_ai_enabled_ = true;

    mutable std::shared_mutex catalog_mtx_;
    // catalog: hash → CatalogEntry (global, filtered by tenant_id at query time)
    std::unordered_map<std::string, CatalogEntry> catalog_;
    std::vector<ShadowAIEvent> shadow_ai_events_;

    std::string catalog_path_;
    int flush_interval_seconds_ = 300;

    std::atomic<bool> running_{false};
    std::jthread flush_thread_;
    std::atomic<bool> dirty_{false};

    std::atomic<int64_t> total_classified_{0};
    std::atomic<int64_t> shadow_ai_detections_{0};
};
