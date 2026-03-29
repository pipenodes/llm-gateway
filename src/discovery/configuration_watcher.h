#pragma once
#include "dynamic_config.h"
#include "config_validator.h"
#include "../provider_router.h"
#include "../ollama_provider.h"
#include "../openai_custom_provider.h"
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <optional>

namespace discovery {

class ConfigurationWatcher {
public:
    using RouterPtr = std::shared_ptr<ProviderRouter>;

    explicit ConfigurationWatcher(std::vector<std::string> merge_priority = {});
    ~ConfigurationWatcher();

    ConfigurationWatcher(const ConfigurationWatcher&) = delete;
    ConfigurationWatcher& operator=(const ConfigurationWatcher&) = delete;

    void add_provider(std::unique_ptr<IConfigProvider> provider);

    void start();
    void stop();

    [[nodiscard]] RouterPtr current_router() const;
    [[nodiscard]] MergedDynamicConfig current_config() const;
    [[nodiscard]] std::optional<std::string> resolve_model_alias(const std::string& model) const;
    [[nodiscard]] Json::Value discovery_status() const;
    void force_refresh();

private:
    static constexpr int DEBOUNCE_MS = 500;
    static constexpr int MIN_RELOAD_INTERVAL_MS = 1000;

    std::vector<std::unique_ptr<IConfigProvider>> providers_;
    std::vector<std::string> merge_priority_;

    mutable std::shared_mutex router_mu_;
    RouterPtr router_;

    mutable std::mutex config_mu_;
    MergedDynamicConfig current_merged_;

    std::atomic<bool> running_{false};
    std::mutex debounce_mu_;
    std::condition_variable debounce_cv_;
    std::atomic<bool> change_pending_{false};
    std::thread debounce_thread_;
    std::chrono::steady_clock::time_point last_reload_{};

    void on_config_changed();
    void debounce_loop();
    void reload();
    [[nodiscard]] MergedDynamicConfig merge_fragments() const;
    [[nodiscard]] RouterPtr build_router(const MergedDynamicConfig& merged) const;
};

} // namespace discovery
