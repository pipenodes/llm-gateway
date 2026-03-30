#pragma once
#include "plugin.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <fstream>
#include <mutex>
#include <atomic>

// RF-33-FUNCIONAL.md — Data Loss Prevention

enum class DLPAction { Allow, Block, Redact, Alert, Quarantine };

struct DLPTypePolicy {
    std::string data_type;        // "pii", "phi", "pci", "confidential"
    DLPAction   action = DLPAction::Alert;
    double      confidence_threshold = 0.8;
};

struct DLPTenantPolicy {
    std::vector<DLPTypePolicy> type_policies;
    bool log_detections = true;
    bool audit_trail    = true;
};

struct QuarantineEntry {
    std::string request_id;
    std::string tenant_id;
    std::string app_id;
    std::string client_id;
    std::string tag;
    int64_t     timestamp = 0;
    std::string reason;
};

class DLPPlugin : public Plugin {
public:
    std::string name() const override { return "dlp"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;
    void shutdown() override;
    Json::Value stats() const override;

    Json::Value get_quarantine(const std::string& tenant_id) const;

private:
    DLPTenantPolicy load_policy(const Json::Value& cfg) const;
    const DLPTenantPolicy& get_policy(const std::string& tenant,
                                       const std::string& app) const;
    DLPAction resolve_action(const DLPTenantPolicy& policy, const std::string& tag) const;
    void write_audit(const PluginRequestContext& ctx, const std::string& tag,
                     DLPAction action, const std::string& source) const;
    void add_quarantine(const PluginRequestContext& ctx, const std::string& tag,
                        const std::string& reason);
    std::string redact_body(Json::Value& body) const;
    static PluginResult apply_dlp(DLPPlugin* self, const std::vector<std::string>& tags,
                                   const DLPTenantPolicy& policy, Json::Value& body,
                                   PluginRequestContext& ctx, const std::string& source);

    DLPTenantPolicy default_policy_;
    std::unordered_map<std::string, DLPTenantPolicy> tenant_policies_; // "tenant:app"

    std::string audit_path_;
    std::string quarantine_path_;
    mutable std::mutex audit_mtx_;

    mutable std::shared_mutex quarantine_mtx_;
    std::vector<QuarantineEntry> quarantine_;

    std::atomic<int64_t> total_inspected_{0};
    std::atomic<int64_t> blocked_{0};
    std::atomic<int64_t> redacted_{0};
    std::atomic<int64_t> quarantined_{0};
};
