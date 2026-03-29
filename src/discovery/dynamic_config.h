#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <json/json.h>

namespace discovery {

struct DiscoveredBackend {
    std::string name;
    std::string qualified_id;
    std::string type;
    std::string base_url;
    std::string api_key_env;
    std::string default_model;
    int weight = 1;
    std::vector<std::string> models;

    [[nodiscard]] Json::Value to_json() const {
        Json::Value j;
        j["name"] = name;
        j["qualified_id"] = qualified_id;
        j["type"] = type;
        j["base_url"] = base_url;
        if (!api_key_env.empty()) j["api_key_env"] = api_key_env;
        if (!default_model.empty()) j["default_model"] = default_model;
        j["weight"] = weight;
        Json::Value arr(Json::arrayValue);
        for (const auto& m : models) arr.append(m);
        j["models"] = arr;
        return j;
    }
};

struct DynamicConfigFragment {
    std::string provider;
    std::vector<DiscoveredBackend> backends;
    std::unordered_map<std::string, std::string> model_routing;
    std::unordered_map<std::string, std::string> model_aliases;
    std::vector<std::string> fallback_chain;

    [[nodiscard]] Json::Value to_json() const {
        Json::Value j;
        j["provider"] = provider;
        Json::Value bs(Json::arrayValue);
        for (const auto& b : backends) bs.append(b.to_json());
        j["backends"] = bs;
        Json::Value mr(Json::objectValue);
        for (const auto& [k, v] : model_routing) mr[k] = v;
        j["model_routing"] = mr;
        Json::Value ma(Json::objectValue);
        for (const auto& [k, v] : model_aliases) ma[k] = v;
        j["model_aliases"] = ma;
        Json::Value fc(Json::arrayValue);
        for (const auto& f : fallback_chain) fc.append(f);
        j["fallback_chain"] = fc;
        return j;
    }
};

struct MergedDynamicConfig {
    std::vector<DiscoveredBackend> backends;
    std::unordered_map<std::string, std::string> model_routing;
    std::unordered_map<std::string, std::string> model_aliases;
    std::vector<std::string> fallback_chain;
    std::unordered_map<std::string, DynamicConfigFragment> fragments_by_provider;
    std::chrono::steady_clock::time_point last_reload{};

    [[nodiscard]] bool empty() const {
        return backends.empty() && model_routing.empty()
            && model_aliases.empty() && fallback_chain.empty();
    }

    [[nodiscard]] Json::Value to_json() const {
        Json::Value j;
        Json::Value bs(Json::objectValue);
        for (const auto& b : backends) bs[b.qualified_id] = b.to_json();
        j["backends"] = bs;
        Json::Value mr(Json::objectValue);
        for (const auto& [k, v] : model_routing) mr[k] = v;
        j["model_routing"] = mr;
        Json::Value ma(Json::objectValue);
        for (const auto& [k, v] : model_aliases) ma[k] = v;
        j["model_aliases"] = ma;
        Json::Value fc(Json::arrayValue);
        for (const auto& f : fallback_chain) fc.append(f);
        j["fallback_chain"] = fc;
        Json::Value pc(Json::arrayValue);
        for (const auto& [k, _] : fragments_by_provider) pc.append(k);
        j["providers_contributing"] = pc;
        return j;
    }
};

struct ProviderStatus {
    std::string name;
    bool healthy = false;
    std::string last_error;
    int backends_discovered = 0;
    std::chrono::steady_clock::time_point last_refresh{};
    std::chrono::milliseconds last_poll_latency{0};
};

class IConfigProvider {
public:
    virtual ~IConfigProvider() = default;
    [[nodiscard]] virtual std::string name() const = 0;
    [[nodiscard]] virtual DynamicConfigFragment provide() = 0;
    virtual void watch(std::function<void()> on_change) = 0;
    virtual void stop() = 0;
    [[nodiscard]] virtual ProviderStatus status() const = 0;
};

} // namespace discovery
