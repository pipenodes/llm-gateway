#include "docker_provider.h"
#include "../logger.h"
#include "../crypto.h"
#include <sstream>
#include <curl/curl.h>

namespace discovery {

DockerProvider::DockerProvider(DockerProviderConfig config)
    : config_(std::move(config)) {
    status_.name = "docker";
}

DockerProvider::~DockerProvider() { stop(); }

bool DockerProvider::is_unix_socket(const std::string& endpoint) {
    return endpoint.find("unix://") == 0;
}

static size_t curl_write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}

std::string DockerProvider::http_get_unix(const std::string& socket_path,
                                          const std::string& url) {
    std::string response;
    CURL* curl = curl_easy_init();
    if (!curl) return {};

    curl_easy_setopt(curl, CURLOPT_UNIX_SOCKET_PATH, socket_path.c_str());
    std::string full_url = "http://localhost" + url;
    curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        Json::Value f;
        f["error"] = curl_easy_strerror(res);
        Logger::error("discovery_docker_curl_error", f);
        return {};
    }

    return response;
}

std::string DockerProvider::http_get_tcp(const std::string& host, int port,
                                         const std::string& path) {
    std::string response;
    CURL* curl = curl_easy_init();
    if (!curl) return {};

    std::string url = "http://" + host + ":" + std::to_string(port) + path;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) return {};
    return response;
}

std::string DockerProvider::get_label(const Json::Value& labels,
                                      const std::string& key) const {
    std::string full_key = config_.label_prefix + "." + key;
    if (labels.isMember(full_key))
        return labels[full_key].asString();
    return {};
}

std::string DockerProvider::fetch_containers_json() const {
    std::string filter = "{\"label\":[\"" + config_.label_prefix + ".enable=true\"],"
                         "\"status\":[\"running\"]}";

    std::string url = "/containers/json?filters=" + filter;

    if (is_unix_socket(config_.endpoint)) {
        std::string socket_path = config_.endpoint.substr(7); // strip "unix://"
        return http_get_unix(socket_path, url);
    }

    std::string host = config_.endpoint;
    int port = 2375;
    if (host.find("tcp://") == 0) host = host.substr(6);
    auto colon = host.find(':');
    if (colon != std::string::npos) {
        port = std::stoi(host.substr(colon + 1));
        host = host.substr(0, colon);
    }
    return http_get_tcp(host, port, url);
}

DynamicConfigFragment DockerProvider::parse_containers(
    const std::string& json_str) const {
    DynamicConfigFragment frag;
    frag.provider = "docker";

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    auto reader = builder.newCharReader();
    if (!reader->parse(json_str.data(), json_str.data() + json_str.size(),
                       &root, &errs) || !root.isArray()) {
        if (!json_str.empty()) {
            Json::Value f;
            f["error"] = errs;
            Logger::error("discovery_docker_parse_error", f);
        }
        return frag;
    }

    for (const auto& container : root) {
        auto& labels = container["Labels"];
        if (!labels.isObject()) continue;

        std::string enable = get_label(labels, "enable");
        if (enable != "true") continue;

        DiscoveredBackend b;
        b.name = get_label(labels, "provider.name");
        if (b.name.empty()) {
            auto names = container["Names"];
            if (names.isArray() && !names.empty()) {
                b.name = names[0u].asString();
                if (!b.name.empty() && b.name[0] == '/')
                    b.name = b.name.substr(1);
            }
        }
        if (b.name.empty()) continue;

        b.qualified_id = b.name + "@docker";
        b.type = get_label(labels, "provider.type");
        if (b.type.empty()) b.type = "ollama";

        std::string port_str = get_label(labels, "provider.port");
        int port = b.type == "ollama" ? 11434 : 80;
        if (!port_str.empty()) port = std::stoi(port_str);

        // Derive backend URL from container IP
        std::string ip;
        if (container.isMember("NetworkSettings")
            && container["NetworkSettings"].isMember("Networks")
            && container["NetworkSettings"]["Networks"].isObject()) {
            auto& nets = container["NetworkSettings"]["Networks"];
            if (!config_.network.empty() && nets.isMember(config_.network)) {
                ip = nets[config_.network]["IPAddress"].asString();
            } else {
                for (const auto& net_name : nets.getMemberNames()) {
                    auto candidate = nets[net_name]["IPAddress"].asString();
                    if (!candidate.empty()) {
                        ip = candidate;
                        break;
                    }
                }
            }
        }
        if (ip.empty()) continue;

        b.base_url = "http://" + ip + ":" + std::to_string(port);

        b.api_key_env = get_label(labels, "provider.api-key-env");
        b.default_model = get_label(labels, "provider.default-model");

        std::string weight_str = get_label(labels, "provider.weight");
        if (!weight_str.empty()) b.weight = std::stoi(weight_str);

        std::string models_str = get_label(labels, "provider.models");
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

        // model routing labels: hermes.model-routing.<model>=<backend>
        for (const auto& lbl : labels.getMemberNames()) {
            std::string prefix = config_.label_prefix + ".model-routing.";
            if (lbl.find(prefix) == 0) {
                std::string model = lbl.substr(prefix.size());
                frag.model_routing[model] = labels[lbl].asString();
            }
        }
    }

    return frag;
}

DynamicConfigFragment DockerProvider::provide() {
    auto start = std::chrono::steady_clock::now();
    DynamicConfigFragment frag;
    frag.provider = "docker";

    try {
        auto json_str = fetch_containers_json();
        frag = parse_containers(json_str);

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

void DockerProvider::poll_loop() {
    while (running_.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(
            std::chrono::seconds(config_.poll_interval_seconds));
        if (!running_.load(std::memory_order_relaxed)) break;

        try {
            auto json_str = fetch_containers_json();
            auto hash = crypto::sha256_hex(json_str);

            if (hash != last_snapshot_hash_) {
                last_snapshot_hash_ = hash;
                Json::Value f;
                f["hash"] = hash.substr(0, 12);
                Logger::info("discovery_docker_change_detected", f);
                if (on_change_) on_change_();
            }
        } catch (const std::exception& e) {
            std::lock_guard lk(status_mu_);
            status_.healthy = false;
            status_.last_error = e.what();
        }
    }
}

void DockerProvider::watch(std::function<void()> on_change) {
    on_change_ = std::move(on_change);
    running_.store(true, std::memory_order_relaxed);
    watch_thread_ = std::thread(&DockerProvider::poll_loop, this);
}

void DockerProvider::stop() {
    running_.store(false, std::memory_order_relaxed);
    if (watch_thread_.joinable())
        watch_thread_.join();
}

ProviderStatus DockerProvider::status() const {
    std::lock_guard lk(status_mu_);
    return status_;
}

} // namespace discovery
