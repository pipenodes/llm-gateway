#include "configuration_watcher.h"
#include "../logger.h"
#include <algorithm>
#include <unordered_set>

namespace discovery {

ConfigurationWatcher::ConfigurationWatcher(std::vector<std::string> merge_priority)
    : merge_priority_(std::move(merge_priority)) {
    if (merge_priority_.empty())
        merge_priority_ = {"file", "kubernetes", "docker"};
}

ConfigurationWatcher::~ConfigurationWatcher() { stop(); }

void ConfigurationWatcher::add_provider(std::unique_ptr<IConfigProvider> provider) {
    providers_.push_back(std::move(provider));
}

void ConfigurationWatcher::start() {
    if (providers_.empty()) return;

    running_.store(true, std::memory_order_relaxed);

    reload();

    debounce_thread_ = std::thread(&ConfigurationWatcher::debounce_loop, this);

    for (auto& p : providers_) {
        p->watch([this]() { on_config_changed(); });
    }

    Json::Value f;
    f["providers"] = static_cast<int>(providers_.size());
    Logger::info("discovery_watcher_started", f);
}

void ConfigurationWatcher::stop() {
    running_.store(false, std::memory_order_relaxed);

    for (auto& p : providers_)
        p->stop();

    debounce_cv_.notify_all();
    if (debounce_thread_.joinable())
        debounce_thread_.join();

    Logger::info("discovery_watcher_stopped");
}

ConfigurationWatcher::RouterPtr ConfigurationWatcher::current_router() const {
    std::shared_lock lk(router_mu_);
    return router_;
}

MergedDynamicConfig ConfigurationWatcher::current_config() const {
    std::lock_guard lk(config_mu_);
    return current_merged_;
}

std::optional<std::string> ConfigurationWatcher::resolve_model_alias(
    const std::string& model) const {
    std::lock_guard lk(config_mu_);
    auto it = current_merged_.model_aliases.find(model);
    if (it != current_merged_.model_aliases.end())
        return it->second;
    return std::nullopt;
}

Json::Value ConfigurationWatcher::discovery_status() const {
    Json::Value result(Json::objectValue);
    Json::Value providers_arr(Json::arrayValue);

    for (const auto& p : providers_) {
        auto s = p->status();
        Json::Value pj;
        pj["name"] = s.name;
        pj["healthy"] = s.healthy;
        if (!s.last_error.empty()) pj["last_error"] = s.last_error;
        pj["backends_discovered"] = s.backends_discovered;
        pj["last_poll_latency_ms"] = static_cast<Json::Int64>(
            s.last_poll_latency.count());
        providers_arr.append(pj);
    }

    result["providers"] = providers_arr;

    {
        std::lock_guard lk(config_mu_);
        result["total_backends"] = static_cast<int>(current_merged_.backends.size());
        result["total_routes"] = static_cast<int>(current_merged_.model_routing.size());
        result["total_aliases"] = static_cast<int>(current_merged_.model_aliases.size());
    }

    return result;
}

void ConfigurationWatcher::force_refresh() {
    Logger::info("discovery_force_refresh");
    reload();
}

void ConfigurationWatcher::on_config_changed() {
    change_pending_.store(true, std::memory_order_relaxed);
    debounce_cv_.notify_one();
}

void ConfigurationWatcher::debounce_loop() {
    while (running_.load(std::memory_order_relaxed)) {
        {
            std::unique_lock lk(debounce_mu_);
            debounce_cv_.wait(lk, [this]() {
                return change_pending_.load(std::memory_order_relaxed)
                    || !running_.load(std::memory_order_relaxed);
            });
        }

        if (!running_.load(std::memory_order_relaxed)) break;

        std::this_thread::sleep_for(std::chrono::milliseconds(DEBOUNCE_MS));

        change_pending_.store(false, std::memory_order_relaxed);

        auto now = std::chrono::steady_clock::now();
        auto since_last = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_reload_);
        if (since_last.count() < MIN_RELOAD_INTERVAL_MS) {
            auto wait = std::chrono::milliseconds(MIN_RELOAD_INTERVAL_MS)
                - since_last;
            std::this_thread::sleep_for(wait);
        }

        if (running_.load(std::memory_order_relaxed))
            reload();
    }
}

void ConfigurationWatcher::reload() {
    auto merged = merge_fragments();

    auto validation = ConfigValidator::validate(merged);

    if (!validation.valid) {
        Json::Value f;
        for (const auto& e : validation.errors) {
            Json::Value err;
            err["error"] = e;
            Logger::error("discovery_validation_error", err);
        }
        Logger::warn("discovery_config_rejected_keeping_previous");
        return;
    }

    for (const auto& w : validation.warnings) {
        Json::Value wf;
        wf["warning"] = w;
        Logger::warn("discovery_validation_warning", wf);
    }

    auto new_router = build_router(merged);

    {
        std::unique_lock lk(router_mu_);
        router_ = new_router;
    }

    {
        std::lock_guard lk(config_mu_);
        current_merged_ = std::move(merged);
        current_merged_.last_reload = std::chrono::steady_clock::now();
    }

    last_reload_ = std::chrono::steady_clock::now();

    Json::Value f;
    f["backends"] = static_cast<int>(new_router ? 1 : 0);
    {
        std::lock_guard lk(config_mu_);
        f["backends"] = static_cast<int>(current_merged_.backends.size());
        f["routes"] = static_cast<int>(current_merged_.model_routing.size());
        f["aliases"] = static_cast<int>(current_merged_.model_aliases.size());
    }
    Logger::info("discovery_config_applied", f);
}

MergedDynamicConfig ConfigurationWatcher::merge_fragments() const {
    // Collect fragments from all providers
    std::vector<DynamicConfigFragment> fragments;
    fragments.reserve(providers_.size());

    for (auto& p : providers_) {
        try {
            fragments.push_back(p->provide());
        } catch (const std::exception& e) {
            Json::Value f;
            f["provider"] = p->name();
            f["error"] = e.what();
            Logger::error("discovery_provider_error", f);
        }
    }

    // Sort fragments by merge_priority_ (highest priority last so it overwrites)
    auto priority_of = [this](const std::string& name) -> int {
        for (int i = 0; i < static_cast<int>(merge_priority_.size()); ++i) {
            if (merge_priority_[i] == name)
                return static_cast<int>(merge_priority_.size()) - i;
        }
        return 0;
    };

    std::sort(fragments.begin(), fragments.end(),
        [&](const DynamicConfigFragment& a, const DynamicConfigFragment& b) {
            return priority_of(a.provider) < priority_of(b.provider);
        });

    MergedDynamicConfig merged;

    for (auto& frag : fragments) {
        // Backends: accumulate (union), keyed by qualified_id
        for (auto& b : frag.backends) {
            bool exists = false;
            for (auto& existing : merged.backends) {
                if (existing.qualified_id == b.qualified_id) {
                    existing = std::move(b);
                    exists = true;
                    break;
                }
            }
            if (!exists)
                merged.backends.push_back(std::move(b));
        }

        // Model routing: higher priority overwrites
        for (auto& [k, v] : frag.model_routing)
            merged.model_routing[k] = std::move(v);

        // Model aliases: higher priority overwrites
        for (auto& [k, v] : frag.model_aliases)
            merged.model_aliases[k] = std::move(v);

        // Fallback chain: higher priority replaces entirely
        if (!frag.fallback_chain.empty())
            merged.fallback_chain = std::move(frag.fallback_chain);

        merged.fragments_by_provider[frag.provider] = frag;
    }

    return merged;
}

ConfigurationWatcher::RouterPtr
ConfigurationWatcher::build_router(const MergedDynamicConfig& merged) const {
    if (merged.empty()) return nullptr;

    auto router = std::make_shared<ProviderRouter>();

    for (const auto& b : merged.backends) {
        if (b.type == "ollama") {
            auto provider = std::make_unique<OllamaProvider>(b.default_model, b.qualified_id);
            // Parse host:port from base_url
            std::string url = b.base_url;
            if (url.find("http://") == 0) url = url.substr(7);
            else if (url.find("https://") == 0) url = url.substr(8);
            while (!url.empty() && url.back() == '/') url.pop_back();

            std::string host = url;
            int port = 11434;
            auto colon = url.find(':');
            if (colon != std::string::npos) {
                host = url.substr(0, colon);
                try { port = std::stoi(url.substr(colon + 1)); }
                catch (...) { port = 11434; }
            }

            std::vector<std::pair<std::string, int>> backends = {{host, port}};
            provider->init(backends);
            router->register_provider(std::move(provider));
        } else if (b.type == "openai") {
            OpenAICustomProvider::Config oc;
            oc.id = b.qualified_id;
            oc.base_url = b.base_url;
            oc.api_key_env = b.api_key_env;
            oc.default_model = b.default_model;
            router->register_provider(std::make_unique<OpenAICustomProvider>(oc));
        }
    }

    // Model routing: resolve qualified_id or name
    for (const auto& [model, target] : merged.model_routing) {
        // Try qualified_id first, then name
        std::string resolved = target;
        for (const auto& b : merged.backends) {
            if (b.name == target || b.qualified_id == target) {
                resolved = b.qualified_id;
                break;
            }
        }
        router->set_model_mapping(model, resolved);
    }

    // Fallback chain: resolve to qualified_ids
    std::vector<std::string> resolved_chain;
    for (const auto& entry : merged.fallback_chain) {
        for (const auto& b : merged.backends) {
            if (b.name == entry || b.qualified_id == entry) {
                resolved_chain.push_back(b.qualified_id);
                break;
            }
        }
    }
    if (!resolved_chain.empty())
        router->set_fallback_chain(std::move(resolved_chain));

    return router;
}

} // namespace discovery
