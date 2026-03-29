#pragma once
#include "dynamic_config.h"
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

namespace discovery {

struct DockerProviderConfig {
    std::string endpoint = "unix:///var/run/docker.sock";
    int poll_interval_seconds = 5;
    std::string label_prefix = "hermes";
    std::string network;
};

class DockerProvider : public IConfigProvider {
public:
    explicit DockerProvider(DockerProviderConfig config);
    ~DockerProvider() override;

    [[nodiscard]] std::string name() const override { return "docker"; }
    [[nodiscard]] DynamicConfigFragment provide() override;
    void watch(std::function<void()> on_change) override;
    void stop() override;
    [[nodiscard]] ProviderStatus status() const override;

private:
    DockerProviderConfig config_;
    std::function<void()> on_change_;
    std::thread watch_thread_;
    std::atomic<bool> running_{false};

    mutable std::mutex status_mu_;
    ProviderStatus status_;

    std::string last_snapshot_hash_;

    void poll_loop();
    [[nodiscard]] std::string fetch_containers_json() const;
    [[nodiscard]] DynamicConfigFragment parse_containers(const std::string& json_str) const;
    [[nodiscard]] std::string get_label(const Json::Value& labels,
                                        const std::string& key) const;

    static bool is_unix_socket(const std::string& endpoint);
    static std::string http_get_unix(const std::string& socket_path,
                                     const std::string& url);
    static std::string http_get_tcp(const std::string& host, int port,
                                    const std::string& path);
};

} // namespace discovery
