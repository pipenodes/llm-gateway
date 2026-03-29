#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <charconv>
#include <json/json.h>
#include "logger.h"

struct ProviderConfig {
    bool enabled = false;
    std::string default_model;
    std::string base_url;
    std::string api_key_env;
    std::string id;
    std::vector<std::pair<std::string, int>> backends;
};

struct Config {
    static constexpr int DEFAULT_PORT = 8080;
    static constexpr int DEFAULT_OLLAMA_PORT = 11434;

    int port = DEFAULT_PORT;
    std::vector<std::pair<std::string, int>> ollama_backends;
    std::string admin_key;
    std::vector<std::string> admin_ip_whitelist;
    std::unordered_map<std::string, std::string> model_aliases;
    int rate_limit_rpm = 0;
    bool cache_enabled = false;
    int cache_ttl = 300;
    size_t cache_max_entries = 1000;
    size_t cache_max_memory_mb = 0;
    bool docs_enabled = false;
    std::string cors_origin = "*";
    size_t max_payload_mb = 1;

    bool multi_provider_enabled = false;
    std::unordered_map<std::string, ProviderConfig> providers;
    std::unordered_map<std::string, std::string> model_routing;
    std::vector<std::string> fallback_chain;

    bool plugins_enabled = false;
    std::string plugins_dir = "plugins/";
    Json::Value plugins_config;

    bool queue_enabled = false;
    size_t queue_max_size = 1000;
    size_t queue_max_concurrency = 4;
    int queue_timeout_ms = 60000;
    size_t queue_worker_threads = 4;

    std::string update_check_url;
    /** Path para gravar o binário baixado (ex: /tmp/hermes.new). Vazio = apply desabilitado. */
    std::string update_staged_binary_path;
    /** Código de saída após apply para o supervisor reiniciar (0 = não sair). */
    int update_exit_code_for_restart = 0;

    struct DiscoveryDockerConfig {
        bool enabled = false;
        std::string endpoint = "unix:///var/run/docker.sock";
        int poll_interval_seconds = 5;
        std::string label_prefix = "hermes";
        std::string network;
    };
    struct DiscoveryKubernetesConfig {
        bool enabled = false;
        std::string api_server;
        std::string namespace_name;
        std::string label_selector = "hermes.io/enable=true";
        int poll_interval_seconds = 10;
        bool use_watch = true;
    };
    struct DiscoveryFileConfig {
        bool enabled = false;
        std::vector<std::string> paths;
        bool watch = true;
        int poll_interval_seconds = 5;
    };
    struct DiscoveryConfig {
        DiscoveryDockerConfig docker;
        DiscoveryKubernetesConfig kubernetes;
        DiscoveryFileConfig file;
        std::vector<std::string> merge_priority = {"file", "kubernetes", "docker"};
        [[nodiscard]] bool any_enabled() const {
            return docker.enabled || kubernetes.enabled || file.enabled;
        }
    };
    DiscoveryConfig discovery;

    void load(const std::string& config_file) {
        std::ifstream file(config_file);
        if (file.is_open()) {
            Json::Value root;
            Json::CharReaderBuilder builder;
            std::string errs;
            if (Json::parseFromStream(builder, file, &root, &errs)) {
                loadGateway(root);
                loadOllama(root);
                loadAliases(root);
                loadCache(root);
                loadDocs(root);
                loadProviders(root);
                loadPlugins(root);
                loadQueue(root);
                loadUpdates(root);
                loadDiscovery(root);
            } else {
                Json::Value f; f["detail"] = errs;
                Logger::error("config_parse_error", f);
            }
        } else {
            Logger::info("config_defaults");
        }

        loadEnvOverrides();

        if (ollama_backends.empty())
            ollama_backends.emplace_back("localhost", DEFAULT_OLLAMA_PORT);
    }

    [[nodiscard]] std::string resolveModel(std::string_view model) const {
        auto it = model_aliases.find(std::string(model));
        return (it != model_aliases.end()) ? it->second : std::string(model);
    }

private:
    void loadGateway(const Json::Value& root) {
        if (!root.isMember("gateway")) return;
        auto& gw = root["gateway"];
        if (gw.isMember("port")) port = gw["port"].asInt();
        if (gw.isMember("cors_origin")) cors_origin = gw["cors_origin"].asString();
        if (gw.isMember("max_payload_mb")) max_payload_mb = static_cast<size_t>(gw["max_payload_mb"].asInt());
    }

    void loadOllama(const Json::Value& root) {
        if (!root.isMember("ollama")) return;
        auto& ollama = root["ollama"];

        if (ollama.isMember("urls") && ollama["urls"].isArray()) {
            for (const auto& u : ollama["urls"])
                parseAndAddBackend(u.asString());
        } else if (ollama.isMember("url")) {
            parseAndAddBackend(ollama["url"].asString());
        }
    }

    void loadAliases(const Json::Value& root) {
        if (!root.isMember("model_aliases") || !root["model_aliases"].isObject()) return;
        for (const auto& key : root["model_aliases"].getMemberNames())
            model_aliases[key] = root["model_aliases"][key].asString();
    }

    void loadCache(const Json::Value& root) {
        if (!root.isMember("cache") || !root["cache"].isObject()) return;
        auto& c = root["cache"];
        if (c.isMember("enabled")) cache_enabled = c["enabled"].asBool();
        if (c.isMember("ttl")) cache_ttl = c["ttl"].asInt();
        if (c.isMember("max_entries")) cache_max_entries = static_cast<size_t>(c["max_entries"].asInt());
        if (c.isMember("max_memory_mb")) cache_max_memory_mb = static_cast<size_t>(c["max_memory_mb"].asInt());
    }

    void loadDocs(const Json::Value& root) {
        if (!root.isMember("docs") || !root["docs"].isObject()) return;
        if (root["docs"].isMember("enabled"))
            docs_enabled = root["docs"]["enabled"].asBool();
    }

    void loadPlugins(const Json::Value& root) {
        if (!root.isMember("plugins") || !root["plugins"].isObject()) return;
        auto& p = root["plugins"];
        if (p.isMember("enabled")) plugins_enabled = p["enabled"].asBool();
        if (p.isMember("dynamic_plugins_dir"))
            plugins_dir = p["dynamic_plugins_dir"].asString();
        plugins_config = p;
    }

    void loadQueue(const Json::Value& root) {
        if (!root.isMember("queue") || !root["queue"].isObject()) return;
        auto& q = root["queue"];
        if (q.isMember("enabled")) queue_enabled = q["enabled"].asBool();
        if (q.isMember("max_size")) queue_max_size = static_cast<size_t>(q["max_size"].asInt());
        if (q.isMember("max_concurrency")) queue_max_concurrency = static_cast<size_t>(q["max_concurrency"].asInt());
        if (q.isMember("default_timeout_ms")) queue_timeout_ms = q["default_timeout_ms"].asInt();
        if (q.isMember("worker_threads")) queue_worker_threads = static_cast<size_t>(q["worker_threads"].asInt());
    }

    void loadUpdates(const Json::Value& root) {
        if (!root.isMember("updates") || !root["updates"].isObject()) return;
        auto& u = root["updates"];
        if (u.isMember("check_url")) update_check_url = u["check_url"].asString();
        if (u.isMember("staged_binary_path")) update_staged_binary_path = u["staged_binary_path"].asString();
        if (u.isMember("exit_code_for_restart")) update_exit_code_for_restart = u["exit_code_for_restart"].asInt();
    }

    void loadProviders(const Json::Value& root) {
        if (!root.isMember("providers") || !root["providers"].isObject()) return;

        auto& prov = root["providers"];

        if (prov.isMember("ollama") && prov["ollama"].isObject()) {
            auto& o = prov["ollama"];
            ProviderConfig pc;
            pc.enabled = o.get("enabled", true).asBool();
            pc.default_model = o.get("default_model", "llama3:8b").asString();
            if (o.isMember("backends") && o["backends"].isArray()) {
                for (const auto& u : o["backends"])
                    parseAndAddBackendTo(pc.backends, u.asString());
            }
            if (pc.backends.empty() && !ollama_backends.empty())
                pc.backends = ollama_backends;
            providers["ollama"] = std::move(pc);
        } else if (!ollama_backends.empty()) {
            ProviderConfig pc;
            pc.enabled = true;
            pc.default_model = "llama3:8b";
            pc.backends = ollama_backends;
            providers["ollama"] = std::move(pc);
        }

        if (prov.isMember("openrouter") && prov["openrouter"].isObject()) {
            auto& o = prov["openrouter"];
            ProviderConfig pc;
            pc.enabled = o.get("enabled", false).asBool();
            pc.default_model = o.get("default_model", "openai/gpt-4o-mini").asString();
            pc.base_url = o.get("base_url", "https://openrouter.ai/api/v1").asString();
            pc.api_key_env = o.get("api_key_env", "OPENROUTER_API_KEY").asString();
            pc.id = "openrouter";
            providers["openrouter"] = std::move(pc);
        }

        if (prov.isMember("custom") && prov["custom"].isObject()) {
            auto& o = prov["custom"];
            ProviderConfig pc;
            pc.enabled = o.get("enabled", false).asBool();
            pc.default_model = o.get("default_model", "gpt-4").asString();
            pc.base_url = o.get("base_url", "").asString();
            pc.api_key_env = o.get("api_key_env", "CUSTOM_OPENAI_API_KEY").asString();
            pc.id = o.get("id", "custom-openai").asString();
            if (!pc.base_url.empty())
                providers["custom"] = std::move(pc);
        }

        if (root.isMember("model_routing") && root["model_routing"].isObject()) {
            for (const auto& key : root["model_routing"].getMemberNames())
                model_routing[key] = root["model_routing"][key].asString();
        }

        if (root.isMember("fallback_chain") && root["fallback_chain"].isArray()) {
            fallback_chain.clear();
            for (const auto& p : root["fallback_chain"])
                fallback_chain.push_back(p.asString());
        }
        if (fallback_chain.empty())
            fallback_chain = {"ollama", "openrouter"};

        if (!providers.empty() || !model_routing.empty())
            multi_provider_enabled = true;
    }

    void parseAndAddBackendTo(std::vector<std::pair<std::string, int>>& out,
                              std::string_view url) {
        if (url.starts_with("http://")) url.remove_prefix(7);
        else if (url.starts_with("https://")) url.remove_prefix(8);
        while (url.ends_with('/')) url.remove_suffix(1);

        auto colon = url.find(':');
        if (colon != std::string_view::npos) {
            std::string host(url.substr(0, colon));
            int p = DEFAULT_OLLAMA_PORT;
            auto ps = url.substr(colon + 1);
            auto [ptr, ec] = std::from_chars(ps.data(), ps.data() + ps.size(), p);
            if (ec != std::errc{}) p = DEFAULT_OLLAMA_PORT;
            out.emplace_back(std::move(host), p);
        } else {
            out.emplace_back(std::string(url), DEFAULT_OLLAMA_PORT);
        }
    }

    void loadEnvOverrides() {
        if (const char* env = std::getenv("OLLAMA_URL")) {
            ollama_backends.clear();
            std::string urls = env;
            size_t pos;
            while ((pos = urls.find(',')) != std::string::npos) {
                parseAndAddBackend(urls.substr(0, pos));
                urls.erase(0, pos + 1);
            }
            parseAndAddBackend(urls);
            Json::Value f; f["urls"] = env;
            Logger::info("config_ollama_env", f);
        }

        if (const char* key = std::getenv("ADMIN_KEY")) {
            admin_key = key;
            Logger::info("config_admin_key_set");
        }

        if (const char* env = std::getenv("ADMIN_IP_WHITELIST")) {
            admin_ip_whitelist.clear();
            std::string list = env;
            size_t pos;
            while ((pos = list.find(',')) != std::string::npos) {
                auto item = list.substr(0, pos);
                if (!item.empty()) admin_ip_whitelist.push_back(item);
                list.erase(0, pos + 1);
            }
            if (!list.empty()) admin_ip_whitelist.push_back(list);
        }

        if (const char* env = std::getenv("MODEL_ALIASES")) {
            std::string aliases = env;
            while (!aliases.empty()) {
                auto comma = aliases.find(',');
                auto pair = (comma != std::string::npos) ? aliases.substr(0, comma) : aliases;
                auto eq = pair.find('=');
                if (eq != std::string::npos)
                    model_aliases[pair.substr(0, eq)] = pair.substr(eq + 1);
                if (comma != std::string::npos) aliases.erase(0, comma + 1);
                else break;
            }
            Logger::info("config_aliases_env");
        }

        if (const char* env = std::getenv("RATE_LIMIT")) {
            int val = 0;
            auto [ptr, ec] = std::from_chars(env, env + std::strlen(env), val);
            if (ec == std::errc{} && val > 0) rate_limit_rpm = val;
        }

        if (const char* env = std::getenv("CACHE_ENABLED")) {
            std::string v = env;
            cache_enabled = (v == "true" || v == "1" || v == "yes");
        }

        if (const char* env = std::getenv("CACHE_TTL")) {
            int val = 0;
            auto [ptr, ec] = std::from_chars(env, env + std::strlen(env), val);
            if (ec == std::errc{} && val > 0) cache_ttl = val;
        }

        if (const char* env = std::getenv("CACHE_MAX_ENTRIES")) {
            int val = 0;
            auto [ptr, ec] = std::from_chars(env, env + std::strlen(env), val);
            if (ec == std::errc{} && val > 0) cache_max_entries = static_cast<size_t>(val);
        }

        if (const char* env = std::getenv("CACHE_MAX_MEMORY_MB")) {
            int val = 0;
            auto [ptr, ec] = std::from_chars(env, env + std::strlen(env), val);
            if (ec == std::errc{} && val > 0) cache_max_memory_mb = static_cast<size_t>(val);
        }

        if (const char* env = std::getenv("DOCS_ENABLED")) {
            std::string v = env;
            docs_enabled = (v == "true" || v == "1" || v == "yes");
        }

        if (const char* env = std::getenv("CORS_ORIGIN"))
            cors_origin = env;

        if (const char* env = std::getenv("MAX_PAYLOAD_MB")) {
            int val = 0;
            auto [ptr, ec] = std::from_chars(env, env + std::strlen(env), val);
            if (ec == std::errc{} && val > 0) max_payload_mb = static_cast<size_t>(val);
        }

        if (providers.count("ollama")) {
            if (const char* env = std::getenv("OLLAMA_ENABLED")) {
                std::string v = env;
                providers["ollama"].enabled = (v == "true" || v == "1" || v == "yes");
            }
            if (const char* env = std::getenv("OLLAMA_DEFAULT_MODEL"))
                providers["ollama"].default_model = env;
        }
        if (providers.count("openrouter")) {
            if (const char* env = std::getenv("OPENROUTER_ENABLED")) {
                std::string v = env;
                providers["openrouter"].enabled = (v == "true" || v == "1" || v == "yes");
            }
            if (const char* env = std::getenv("OPENROUTER_DEFAULT_MODEL"))
                providers["openrouter"].default_model = env;
        }
        if (providers.count("custom")) {
            if (const char* env = std::getenv("CUSTOM_PROVIDER_ENABLED")) {
                std::string v = env;
                providers["custom"].enabled = (v == "true" || v == "1" || v == "yes");
            }
            if (const char* env = std::getenv("CUSTOM_PROVIDER_BASE_URL"))
                providers["custom"].base_url = env;
            if (const char* env = std::getenv("CUSTOM_PROVIDER_DEFAULT_MODEL"))
                providers["custom"].default_model = env;
        }
        if (const char* env = std::getenv("PLUGINS_ENABLED")) {
            std::string v = env;
            plugins_enabled = (v == "true" || v == "1" || v == "yes");
        }
        if (const char* env = std::getenv("PLUGINS_DIR"))
            plugins_dir = env;

        if (const char* env = std::getenv("QUEUE_ENABLED")) {
            std::string v = env;
            queue_enabled = (v == "true" || v == "1" || v == "yes");
        }
        if (const char* env = std::getenv("QUEUE_MAX_SIZE")) {
            int val = 0;
            auto [ptr, ec] = std::from_chars(env, env + std::strlen(env), val);
            if (ec == std::errc{} && val > 0) queue_max_size = static_cast<size_t>(val);
        }
        if (const char* env = std::getenv("QUEUE_MAX_CONCURRENCY")) {
            int val = 0;
            auto [ptr, ec] = std::from_chars(env, env + std::strlen(env), val);
            if (ec == std::errc{} && val > 0) queue_max_concurrency = static_cast<size_t>(val);
        }
        if (const char* env = std::getenv("QUEUE_TIMEOUT_MS")) {
            int val = 0;
            auto [ptr, ec] = std::from_chars(env, env + std::strlen(env), val);
            if (ec == std::errc{} && val > 0) queue_timeout_ms = val;
        }
        if (const char* env = std::getenv("QUEUE_WORKERS")) {
            int val = 0;
            auto [ptr, ec] = std::from_chars(env, env + std::strlen(env), val);
            if (ec == std::errc{} && val > 0) queue_worker_threads = static_cast<size_t>(val);
        }

        if (const char* env = std::getenv("UPDATE_CHECK_URL"))
            update_check_url = env;
        if (const char* env = std::getenv("UPDATE_STAGED_BINARY_PATH"))
            update_staged_binary_path = env;
        if (const char* env = std::getenv("UPDATE_EXIT_CODE_FOR_RESTART")) {
            int val = 0;
            auto [ptr, ec] = std::from_chars(env, env + std::strlen(env), val);
            if (ec == std::errc{}) update_exit_code_for_restart = val;
        }

        if (const char* env = std::getenv("DISCOVERY_DOCKER_ENABLED")) {
            std::string v = env;
            discovery.docker.enabled = (v == "true" || v == "1" || v == "yes");
        }
        if (const char* env = std::getenv("DISCOVERY_DOCKER_ENDPOINT"))
            discovery.docker.endpoint = env;
        if (const char* env = std::getenv("DISCOVERY_DOCKER_POLL_INTERVAL")) {
            int val = 0;
            auto [ptr, ec] = std::from_chars(env, env + std::strlen(env), val);
            if (ec == std::errc{} && val > 0) discovery.docker.poll_interval_seconds = val;
        }
        if (const char* env = std::getenv("DISCOVERY_DOCKER_NETWORK"))
            discovery.docker.network = env;

        if (const char* env = std::getenv("DISCOVERY_K8S_ENABLED")) {
            std::string v = env;
            discovery.kubernetes.enabled = (v == "true" || v == "1" || v == "yes");
        }
        if (const char* env = std::getenv("DISCOVERY_K8S_API_SERVER"))
            discovery.kubernetes.api_server = env;
        if (const char* env = std::getenv("DISCOVERY_K8S_NAMESPACE"))
            discovery.kubernetes.namespace_name = env;
        if (const char* env = std::getenv("DISCOVERY_K8S_POLL_INTERVAL")) {
            int val = 0;
            auto [ptr, ec] = std::from_chars(env, env + std::strlen(env), val);
            if (ec == std::errc{} && val > 0) discovery.kubernetes.poll_interval_seconds = val;
        }

        if (const char* env = std::getenv("DISCOVERY_FILE_ENABLED")) {
            std::string v = env;
            discovery.file.enabled = (v == "true" || v == "1" || v == "yes");
        }
        if (const char* env = std::getenv("DISCOVERY_FILE_PATHS")) {
            discovery.file.paths.clear();
            std::string list = env;
            size_t pos;
            while ((pos = list.find(',')) != std::string::npos) {
                auto item = list.substr(0, pos);
                if (!item.empty()) discovery.file.paths.push_back(item);
                list.erase(0, pos + 1);
            }
            if (!list.empty()) discovery.file.paths.push_back(list);
        }
        if (const char* env = std::getenv("DISCOVERY_FILE_POLL_INTERVAL")) {
            int val = 0;
            auto [ptr, ec] = std::from_chars(env, env + std::strlen(env), val);
            if (ec == std::errc{} && val > 0) discovery.file.poll_interval_seconds = val;
        }

        if (const char* env = std::getenv("FALLBACK_CHAIN")) {
            fallback_chain.clear();
            std::string list = env;
            size_t pos;
            while ((pos = list.find(',')) != std::string::npos) {
                auto item = list.substr(0, pos);
                if (!item.empty()) fallback_chain.push_back(item);
                list.erase(0, pos + 1);
            }
            if (!list.empty()) fallback_chain.push_back(list);
        }
    }

    void loadDiscovery(const Json::Value& root) {
        if (!root.isMember("discovery") || !root["discovery"].isObject()) return;
        auto& d = root["discovery"];

        if (d.isMember("docker") && d["docker"].isObject()) {
            auto& dc = d["docker"];
            if (dc.isMember("enabled")) discovery.docker.enabled = dc["enabled"].asBool();
            if (dc.isMember("endpoint")) discovery.docker.endpoint = dc["endpoint"].asString();
            if (dc.isMember("poll_interval_seconds")) discovery.docker.poll_interval_seconds = dc["poll_interval_seconds"].asInt();
            if (dc.isMember("label_prefix")) discovery.docker.label_prefix = dc["label_prefix"].asString();
            if (dc.isMember("network")) discovery.docker.network = dc["network"].asString();
        }

        if (d.isMember("kubernetes") && d["kubernetes"].isObject()) {
            auto& kc = d["kubernetes"];
            if (kc.isMember("enabled")) discovery.kubernetes.enabled = kc["enabled"].asBool();
            if (kc.isMember("api_server")) discovery.kubernetes.api_server = kc["api_server"].asString();
            if (kc.isMember("namespace")) discovery.kubernetes.namespace_name = kc["namespace"].asString();
            if (kc.isMember("label_selector")) discovery.kubernetes.label_selector = kc["label_selector"].asString();
            if (kc.isMember("poll_interval_seconds")) discovery.kubernetes.poll_interval_seconds = kc["poll_interval_seconds"].asInt();
            if (kc.isMember("use_watch")) discovery.kubernetes.use_watch = kc["use_watch"].asBool();
        }

        if (d.isMember("file") && d["file"].isObject()) {
            auto& fc = d["file"];
            if (fc.isMember("enabled")) discovery.file.enabled = fc["enabled"].asBool();
            if (fc.isMember("paths") && fc["paths"].isArray()) {
                discovery.file.paths.clear();
                for (const auto& p : fc["paths"])
                    discovery.file.paths.push_back(p.asString());
            }
            if (fc.isMember("watch")) discovery.file.watch = fc["watch"].asBool();
            if (fc.isMember("poll_interval_seconds")) discovery.file.poll_interval_seconds = fc["poll_interval_seconds"].asInt();
        }

        if (d.isMember("merge_priority") && d["merge_priority"].isArray()) {
            discovery.merge_priority.clear();
            for (const auto& p : d["merge_priority"])
                discovery.merge_priority.push_back(p.asString());
        }
    }

    void parseAndAddBackend(std::string_view url) {
        if (url.starts_with("http://")) url.remove_prefix(7);
        else if (url.starts_with("https://")) url.remove_prefix(8);
        while (url.ends_with('/')) url.remove_suffix(1);

        auto colon = url.find(':');
        if (colon != std::string_view::npos) {
            std::string host(url.substr(0, colon));
            int p = DEFAULT_OLLAMA_PORT;
            auto ps = url.substr(colon + 1);
            auto [ptr, ec] = std::from_chars(ps.data(), ps.data() + ps.size(), p);
            if (ec != std::errc{}) {
                Logger::warn("config_invalid_port");
                p = DEFAULT_OLLAMA_PORT;
            }
            ollama_backends.emplace_back(std::move(host), p);
        } else {
            ollama_backends.emplace_back(std::string(url), DEFAULT_OLLAMA_PORT);
        }
    }
};
