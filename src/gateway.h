#pragma once
#include <string>
#include <string_view>
#include <random>
#include <cstdio>
#include <memory>
#include <thread>
#include <functional>
#include "httplib.h"
#include <json/json.h>
#include "config.h"
#include "logger.h"
#include "metrics_interface.h"
#include "ollama_client.h"
#include "chunk_queue.h"
#include "crypto.h"
#include "plugin.h"
#include "core_services.h"
#include "plugins/logging_plugin.h"
#include "plugins/semantic_cache_plugin.h"
#include "plugins/usage_tracking_plugin.h"
#include "edge_config.h"
#include "dashboard.h"
#if DOCS_ENABLED
#include "openapi_spec.h"
#endif
#if MULTI_PROVIDER
#include "provider.h"
#include "provider_router.h"
#include "ollama_provider.h"
#include "openai_custom_provider.h"
#endif
#if DISCOVERY_ENABLED
#include "discovery/configuration_watcher.h"
#include "discovery/docker_provider.h"
#include "discovery/kubernetes_provider.h"
#include "discovery/file_provider.h"
#endif
#if BENCHMARK_ENABLED
#include "benchmark.h"
#endif

class HermesGateway {
private:
    static constexpr int ID_RANDOM_LENGTH = 24;
    static constexpr std::string_view ID_CHARSET = "abcdefghijklmnopqrstuvwxyz0123456789";
    static constexpr std::string_view HEX_CHARSET = "0123456789abcdef";

    Config config;
    IMetricsCollector* metrics_ = nullptr;
    std::unique_ptr<IMetricsCollector> metrics_impl_;
    OllamaClient ollama;
#if MULTI_PROVIDER
    std::unique_ptr<ProviderRouter> provider_router_;
#endif
#if DISCOVERY_ENABLED
    std::unique_ptr<discovery::ConfigurationWatcher> discovery_watcher_;
#endif
    PluginManager plugin_manager_;
    core::ICache* cache_ = nullptr;
    core::IAuth* auth_ = nullptr;
    core::IRateLimiter* rate_limiter_ = nullptr;
    core::IRequestQueue* queue_ = nullptr;
    std::unique_ptr<core::NullCache> null_cache_;
    std::unique_ptr<core::NullAuth> null_auth_;
    std::unique_ptr<core::NullRateLimiter> null_rate_limiter_;
    std::unique_ptr<core::NullRequestQueue> null_queue_;
    httplib::Server server;

    HermesGateway(const HermesGateway&) = delete;
    HermesGateway& operator=(const HermesGateway&) = delete;

    [[nodiscard]] std::string genId(std::string_view prefix = "chatcmpl-");
    [[nodiscard]] static std::string generateRequestId();

#if MULTI_PROVIDER
    struct ActiveRouter {
        ProviderRouter* ptr = nullptr;
        std::shared_ptr<ProviderRouter> hold;
    };
    [[nodiscard]] ActiveRouter active_router() const;
#endif
    [[nodiscard]] std::string resolveModel(const std::string& model) const;
    void jsonError(httplib::Response& res, int code, std::string_view msg);

    [[nodiscard]] static std::string extractBearer(const httplib::Request& req) noexcept {
        auto auth = req.get_header_value("Authorization");
        if (auth.starts_with("Bearer "))
            return auth.substr(7);
        return {};
    }

    [[nodiscard]] static bool parseJson(const std::string& input, Json::Value& out, std::string& errs);

    [[nodiscard]] bool isPublicPath(const std::string& path) const noexcept {
        return path == "/health"
            || path == "/openapi.json"
            || path == "/docs"
            || path == "/redoc"
            || path == "/dashboard";
    }

    [[nodiscard]] bool isAdminPath(const std::string& path) const noexcept {
        return path.starts_with("/admin/");
    }

    [[nodiscard]] bool isMetricsPath(const std::string& path) const noexcept {
        return path == "/metrics" || path == "/metrics/prometheus";
    }

    [[nodiscard]] static Json::Int64 parseOllamaTimestamp(const std::string& ts);
    static void applyPluginHeaders(httplib::Response& res,
        const PluginRequestContext& pctx);
    static void buildOllamaOptions(const Json::Value& body, Json::Value& oreq);
    static bool applyResponseFormat(const Json::Value& body, Json::Value& oreq,
                                     httplib::Response& res,
                                     std::function<void(httplib::Response&, int, std::string_view)> errFn);

    void setupServer();
    void handleChatCompletions(const httplib::Request& req, httplib::Response& res);
    void handleChatStream(const Json::Value& body,
                          const std::string& resolved_model,
                          const std::string& requested_model,
                          httplib::Response& res,
                          const PluginRequestContext& pctx);
    void handleCompletions(const httplib::Request& req, httplib::Response& res);
    void handleCompletionStream(const Json::Value& body,
                                const std::string& resolved_model,
                                const std::string& requested_model,
                                bool echo,
                                const std::string& prompt,
                                httplib::Response& res,
                                const PluginRequestContext& pctx);
    void handleEmbeddings(const httplib::Request& req, httplib::Response& res);
    void handleListModels(httplib::Response& res);
    void handleRetrieveModel(const httplib::Request& req, httplib::Response& res);
    void handleCreateKey(const httplib::Request& req, httplib::Response& res);
    void handleListKeys(httplib::Response& res);
    void handleRevokeKey(const httplib::Request& req, httplib::Response& res);
    void handleAdminPlugins(httplib::Response& res);
    void handleAdminUpdatesCheck(httplib::Response& res);
    void handleAdminUpdatesApply(const httplib::Request& req, httplib::Response& res);
    void handleAdminQueueGet(httplib::Response& res);
    void handleAdminQueueDelete(httplib::Response& res);
    void handleAdminAudit(const httplib::Request& req, httplib::Response& res);
    void handleAdminPosture(httplib::Response& res);
    void handleAdminUsage(httplib::Response& res);
    void handleAdminUsageByAlias(const httplib::Request& req, httplib::Response& res);
    void handleAdminUsageReset(const httplib::Request& req, httplib::Response& res);
    void handleAdminPromptTemplates(httplib::Response& res);
    void handleAdminPromptTemplatesCreate(const httplib::Request& req, httplib::Response& res);
    void handleAdminPromptKeySet(const httplib::Request& req, httplib::Response& res);
    void handleAdminPromptGuardrails(httplib::Response& res);
    void handleAdminWebhooks(httplib::Response& res);
    void handleAdminWebhooksTest(httplib::Response& res);
    void handleAdminABList(httplib::Response& res);
    void handleAdminABResults(const httplib::Request& req, httplib::Response& res);
    void handleAdminABCreate(const httplib::Request& req, httplib::Response& res);
    void handleAdminABDelete(const httplib::Request& req, httplib::Response& res);
    void handleAdminCosts(httplib::Response& res);
    void handleAdminCostsByAlias(const httplib::Request& req, httplib::Response& res);
    void handleAdminCostsBudgetSet(const httplib::Request& req, httplib::Response& res);
    // Enterprise security + finops plugins (RF-32 to RF-35)
    void handleAdminGuardrailsStats(httplib::Response& res);
    void handleAdminDiscoveryCatalog(const httplib::Request& req, httplib::Response& res);
    void handleAdminDiscoveryShadowAI(const httplib::Request& req, httplib::Response& res);
    void handleAdminDLPQuarantine(const httplib::Request& req, httplib::Response& res);
    void handleAdminFinOpsCosts(const httplib::Request& req, httplib::Response& res);
    void handleAdminFinOpsBudgets(const httplib::Request& req, httplib::Response& res);
    void handleAdminFinOpsBudgetSet(const httplib::Request& req, httplib::Response& res);
    void handleAdminFinOpsExport(httplib::Response& res);
    void handleListSessions(httplib::Response& res);
    void handleGetSession(const httplib::Request& req, httplib::Response& res);
    void handleDeleteSession(const httplib::Request& req, httplib::Response& res);
    void handleAdminComplianceReport(const httplib::Request& req, httplib::Response& res);
    void handleAdminSecurityScan(const httplib::Request& req, httplib::Response& res);
#if DISCOVERY_ENABLED
    void handleAdminDiscovery(httplib::Response& res);
    void handleAdminDiscoveryRefresh(httplib::Response& res);
    void handleAdminConfig(httplib::Response& res);
#endif
#if BENCHMARK_ENABLED
    void handleBenchmark(const httplib::Request& req, httplib::Response& res);
#endif

public:
    explicit HermesGateway(const std::string& configFile = "config.json");
    void stop() noexcept {
        server.stop();
#if DISCOVERY_ENABLED
        if (discovery_watcher_) discovery_watcher_->stop();
#endif
    }
    void start();
};
