#include "kubernetes_provider.h"
#include "../logger.h"
#include "../crypto.h"
#include <fstream>
#include <sstream>
#include <curl/curl.h>

namespace discovery {

KubernetesProvider::KubernetesProvider(KubernetesProviderConfig config)
    : config_(std::move(config)) {
    status_.name = "kubernetes";
    detect_in_cluster();
}

KubernetesProvider::~KubernetesProvider() { stop(); }

void KubernetesProvider::detect_in_cluster() {
    if (!config_.api_server.empty()) return;

    const char* host = std::getenv("KUBERNETES_SERVICE_HOST");
    const char* port = std::getenv("KUBERNETES_SERVICE_PORT");
    if (host && port) {
        config_.api_server = std::string("https://") + host + ":" + port;
        Logger::info("discovery_k8s_in_cluster");
    }

    if (config_.namespace_name.empty()) {
        auto ns = read_file_contents(
            "/var/run/secrets/kubernetes.io/serviceaccount/namespace");
        if (!ns.empty())
            config_.namespace_name = ns;
        else
            config_.namespace_name = "default";
    }
}

std::string KubernetesProvider::read_file_contents(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return {};
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    while (!content.empty() && (content.back() == '\n' || content.back() == '\r'))
        content.pop_back();
    return content;
}

std::string KubernetesProvider::read_token() const {
    return read_file_contents(config_.token_path);
}

static size_t curl_write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}

std::string KubernetesProvider::https_get(const std::string& host, int port,
                                           const std::string& path,
                                           const std::string& token,
                                           const std::string& ca_cert_path) {
    std::string response;
    CURL* curl = curl_easy_init();
    if (!curl) return {};

    std::string url = host;
    if (port > 0 && url.find(":" + std::to_string(port)) == std::string::npos)
        url += ":" + std::to_string(port);
    url += path;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

    struct curl_slist* headers = nullptr;
    if (!token.empty()) {
        std::string auth = "Authorization: Bearer " + token;
        headers = curl_slist_append(headers, auth.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    if (!ca_cert_path.empty()) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, ca_cert_path.c_str());
    } else {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }

    CURLcode res = curl_easy_perform(curl);
    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        Json::Value f;
        f["error"] = curl_easy_strerror(res);
        Logger::error("discovery_k8s_curl_error", f);
        return {};
    }

    return response;
}

std::string KubernetesProvider::get_annotation(const Json::Value& annotations,
                                                const std::string& key) const {
    std::string full_key = "hermes.io/" + key;
    if (annotations.isMember(full_key))
        return annotations[full_key].asString();
    return {};
}

std::string KubernetesProvider::fetch_services_json() const {
    if (config_.api_server.empty()) return {};

    auto token = read_token();
    std::string path = "/api/v1/namespaces/" + config_.namespace_name
        + "/services?labelSelector=" + config_.label_selector;

    std::string host = config_.api_server;
    int port = 0;

    auto scheme_end = host.find("://");
    std::string scheme = "https";
    if (scheme_end != std::string::npos) {
        scheme = host.substr(0, scheme_end);
        host = host.substr(scheme_end + 3);
    }

    auto colon = host.find(':');
    if (colon != std::string::npos) {
        port = std::stoi(host.substr(colon + 1));
        host = host.substr(0, colon);
    } else {
        port = (scheme == "https") ? 443 : 80;
    }

    return https_get(scheme + "://" + host, port, path, token,
                     config_.ca_cert_path);
}

DynamicConfigFragment KubernetesProvider::parse_services(
    const std::string& json_str) const {
    DynamicConfigFragment frag;
    frag.provider = "kubernetes";

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    auto reader = builder.newCharReader();
    if (!reader->parse(json_str.data(), json_str.data() + json_str.size(),
                       &root, &errs)) {
        if (!json_str.empty()) {
            Json::Value f;
            f["error"] = errs;
            Logger::error("discovery_k8s_parse_error", f);
        }
        return frag;
    }

    if (!root.isMember("items") || !root["items"].isArray()) return frag;

    for (const auto& svc : root["items"]) {
        auto& meta = svc["metadata"];
        if (!meta.isObject()) continue;

        auto& annotations = meta["annotations"];
        if (!annotations.isObject()) continue;

        std::string enable = get_annotation(annotations, "enable");
        if (enable != "true") continue;

        DiscoveredBackend b;
        b.name = get_annotation(annotations, "provider-name");
        if (b.name.empty())
            b.name = meta.get("name", "").asString();
        if (b.name.empty()) continue;

        b.qualified_id = b.name + "@kubernetes";
        b.type = get_annotation(annotations, "provider-type");
        if (b.type.empty()) b.type = "ollama";

        // Derive URL from ClusterIP + port annotation
        std::string cluster_ip;
        if (svc.isMember("spec") && svc["spec"].isMember("clusterIP"))
            cluster_ip = svc["spec"]["clusterIP"].asString();
        if (cluster_ip.empty() || cluster_ip == "None") {
            cluster_ip = meta.get("name", "").asString()
                + "." + config_.namespace_name + ".svc.cluster.local";
        }

        std::string port_str = get_annotation(annotations, "port");
        int port = b.type == "ollama" ? 11434 : 80;
        if (!port_str.empty()) port = std::stoi(port_str);

        b.base_url = "http://" + cluster_ip + ":" + std::to_string(port);

        b.api_key_env = get_annotation(annotations, "api-key-env");
        b.default_model = get_annotation(annotations, "default-model");

        std::string weight_str = get_annotation(annotations, "weight");
        if (!weight_str.empty()) b.weight = std::stoi(weight_str);

        std::string models_str = get_annotation(annotations, "models");
        if (!models_str.empty()) {
            std::istringstream ss(models_str);
            std::string model;
            while (std::getline(ss, model, ',')) {
                while (!model.empty() && model[0] == ' ') model.erase(0, 1);
                while (!model.empty() && model.back() == ' ') model.pop_back();
                if (!model.empty()) b.models.push_back(model);
            }
        }

        frag.backends.push_back(std::move(b));

        // Routing annotations
        for (const auto& key : annotations.getMemberNames()) {
            std::string prefix = "hermes.io/model-routing.";
            if (key.find(prefix) == 0) {
                std::string model = key.substr(prefix.size());
                frag.model_routing[model] = annotations[key].asString();
            }
        }
    }

    return frag;
}

DynamicConfigFragment KubernetesProvider::provide() {
    auto start = std::chrono::steady_clock::now();
    DynamicConfigFragment frag;
    frag.provider = "kubernetes";

    try {
        auto json_str = fetch_services_json();
        if (!json_str.empty())
            frag = parse_services(json_str);

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);

        std::lock_guard lk(status_mu_);
        status_.healthy = true;
        status_.last_error.clear();
        status_.backends_discovered = static_cast<int>(frag.backends.size());
        status_.last_refresh = std::chrono::steady_clock::now();
        status_.last_poll_latency = elapsed;
    } catch (const std::exception& e) {
        std::lock_guard lk(status_mu_);
        status_.healthy = false;
        status_.last_error = e.what();
    }

    return frag;
}

void KubernetesProvider::poll_loop() {
    while (running_.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(
            std::chrono::seconds(config_.poll_interval_seconds));
        if (!running_.load(std::memory_order_relaxed)) break;

        try {
            auto json_str = fetch_services_json();
            auto hash = crypto::sha256_hex(json_str);

            if (hash != last_snapshot_hash_) {
                last_snapshot_hash_ = hash;
                Json::Value f;
                f["hash"] = hash.substr(0, 12);
                Logger::info("discovery_k8s_change_detected", f);
                if (on_change_) on_change_();
            }
        } catch (const std::exception& e) {
            std::lock_guard lk(status_mu_);
            status_.healthy = false;
            status_.last_error = e.what();
        }
    }
}

void KubernetesProvider::watch(std::function<void()> on_change) {
    on_change_ = std::move(on_change);
    running_.store(true, std::memory_order_relaxed);
    watch_thread_ = std::thread(&KubernetesProvider::poll_loop, this);
}

void KubernetesProvider::stop() {
    running_.store(false, std::memory_order_relaxed);
    if (watch_thread_.joinable())
        watch_thread_.join();
}

ProviderStatus KubernetesProvider::status() const {
    std::lock_guard lk(status_mu_);
    return status_;
}

} // namespace discovery
