#pragma once
#include "dynamic_config.h"
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

namespace discovery {

struct KubernetesProviderConfig {
    std::string api_server;
    std::string namespace_name;
    std::string label_selector = "hermes.io/enable=true";
    int poll_interval_seconds = 10;
    bool use_watch = true;
    std::string token_path = "/var/run/secrets/kubernetes.io/serviceaccount/token";
    std::string ca_cert_path = "/var/run/secrets/kubernetes.io/serviceaccount/ca.crt";
};

class KubernetesProvider : public IConfigProvider {
public:
    explicit KubernetesProvider(KubernetesProviderConfig config);
    ~KubernetesProvider() override;

    [[nodiscard]] std::string name() const override { return "kubernetes"; }
    [[nodiscard]] DynamicConfigFragment provide() override;
    void watch(std::function<void()> on_change) override;
    void stop() override;
    [[nodiscard]] ProviderStatus status() const override;

private:
    KubernetesProviderConfig config_;
    std::function<void()> on_change_;
    std::thread watch_thread_;
    std::atomic<bool> running_{false};

    mutable std::mutex status_mu_;
    ProviderStatus status_;

    std::string cached_token_;
    std::string last_snapshot_hash_;

    void poll_loop();
    void detect_in_cluster();
    [[nodiscard]] std::string read_token() const;
    [[nodiscard]] std::string fetch_services_json() const;
    [[nodiscard]] DynamicConfigFragment parse_services(const std::string& json_str) const;
    [[nodiscard]] std::string get_annotation(const Json::Value& annotations,
                                              const std::string& key) const;

    static std::string read_file_contents(const std::string& path);
    static std::string https_get(const std::string& host, int port,
                                 const std::string& path,
                                 const std::string& token,
                                 const std::string& ca_cert_path);
};

} // namespace discovery
