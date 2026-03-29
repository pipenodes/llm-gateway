#include "webhook_plugin.h"
#include "../logger.h"
#include "../crypto.h"
#include "../httplib.h"
#include <sstream>

WebhookPlugin::~WebhookPlugin() {
    shutdown();
}

bool WebhookPlugin::init(const Json::Value& config) {
    max_queue_size_ = config.get("max_queue_size", 10000).asUInt();

    if (config.isMember("endpoints") && config["endpoints"].isArray()) {
        for (const auto& ep : config["endpoints"]) {
            WebhookEndpoint endpoint;
            endpoint.name = ep.get("name", "").asString();
            endpoint.url = ep.get("url", "").asString();
            endpoint.max_retries = ep.get("max_retries", 3).asInt();
            endpoint.timeout_ms = ep.get("timeout_ms", 5000).asInt();
            endpoint.enabled = ep.get("enabled", true).asBool();

            if (ep.isMember("event_types") && ep["event_types"].isArray()) {
                for (const auto& et : ep["event_types"])
                    endpoint.event_types.insert(et.asString());
            }

            if (ep.isMember("headers") && ep["headers"].isObject()) {
                for (const auto& h : ep["headers"].getMemberNames())
                    endpoint.headers[h] = ep["headers"][h].asString();
            }

            if (!endpoint.url.empty())
                endpoints_.push_back(std::move(endpoint));
        }
    }

    if (config.isMember("url") && !config["url"].asString().empty()) {
        WebhookEndpoint single;
        single.name = "default";
        single.url = config["url"].asString();
        endpoints_.push_back(std::move(single));
    }

    if (!endpoints_.empty()) {
        running_ = true;
        worker_ = std::jthread([this] { worker_loop(); });
    }

    Json::Value f;
    f["endpoints"] = static_cast<int>(endpoints_.size());
    Logger::info("webhook_init", f);
    return true;
}

void WebhookPlugin::shutdown() {
    if (running_.exchange(false)) {
        cv_.notify_all();
        if (worker_.joinable()) worker_.join();
    }
}

void WebhookPlugin::emit(const std::string& event_type, const Json::Value& data) {
    WebhookEvent event;
    event.type = WebhookEventType::Custom;
    event.type_name = event_type;
    event.timestamp = static_cast<int64_t>(crypto::epoch_seconds());
    event.data = data;

    std::lock_guard lock(mtx_);
    if (event_queue_.size() < max_queue_size_) {
        event_queue_.push(std::move(event));
        cv_.notify_one();
    }
}

void WebhookPlugin::emit(WebhookEventType type, const Json::Value& data) {
    WebhookEvent event;
    event.type = type;
    event.type_name = event_type_to_string(type);
    event.timestamp = static_cast<int64_t>(crypto::epoch_seconds());
    event.data = data;

    std::lock_guard lock(mtx_);
    if (event_queue_.size() < max_queue_size_) {
        event_queue_.push(std::move(event));
        cv_.notify_one();
    }
}

PluginResult WebhookPlugin::before_request(Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult WebhookPlugin::after_response(Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}

Json::Value WebhookPlugin::stats() const {
    std::lock_guard lock(mtx_);
    Json::Value out;
    out["endpoints"] = static_cast<int>(endpoints_.size());
    out["queue_size"] = static_cast<int>(event_queue_.size());

    int64_t total_sent = 0, total_failed = 0;
    for (const auto& ep : endpoints_) {
        total_sent += ep.events_sent;
        total_failed += ep.events_failed;
    }
    out["total_events_sent"] = static_cast<Json::Int64>(total_sent);
    out["total_events_failed"] = static_cast<Json::Int64>(total_failed);
    return out;
}

Json::Value WebhookPlugin::list_endpoints() const {
    std::lock_guard lock(mtx_);
    Json::Value out(Json::arrayValue);
    for (const auto& ep : endpoints_) {
        Json::Value e;
        e["name"] = ep.name;
        e["url"] = mask_url(ep.url);
        e["enabled"] = ep.enabled;
        e["max_retries"] = ep.max_retries;
        e["timeout_ms"] = ep.timeout_ms;
        e["events_sent"] = static_cast<Json::Int64>(ep.events_sent);
        e["events_failed"] = static_cast<Json::Int64>(ep.events_failed);

        Json::Value types(Json::arrayValue);
        for (const auto& t : ep.event_types) types.append(t);
        e["event_types"] = types;
        out.append(e);
    }
    return out;
}

bool WebhookPlugin::send_test_event() {
    Json::Value data;
    data["message"] = "Test webhook event from HERMES gateway";
    data["timestamp"] = static_cast<Json::Int64>(crypto::epoch_seconds());
    emit("test", data);
    return true;
}

void WebhookPlugin::worker_loop() {
    while (running_) {
        WebhookEvent event;
        {
            std::unique_lock lock(mtx_);
            cv_.wait(lock, [this] { return !running_ || !event_queue_.empty(); });
            if (!running_ && event_queue_.empty()) break;
            if (event_queue_.empty()) continue;
            event = std::move(event_queue_.front());
            event_queue_.pop();
        }

        for (auto& ep : endpoints_) {
            if (!ep.enabled) continue;

            if (!ep.event_types.empty() && ep.event_types.find(event.type_name) == ep.event_types.end())
                continue;

            bool sent = false;
            for (int attempt = 0; attempt <= ep.max_retries && !sent; ++attempt) {
                if (attempt > 0) {
                    int backoff_ms = 1000 * (1 << (attempt - 1));
                    std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
                }
                sent = send_to_endpoint(ep, event);
            }

            std::lock_guard lock(mtx_);
            if (sent) {
                ep.events_sent++;
            } else {
                ep.events_failed++;
                Json::Value f;
                f["endpoint"] = ep.name;
                f["event_type"] = event.type_name;
                Logger::warn("webhook_delivery_failed", f);
            }
        }
    }
}

bool WebhookPlugin::send_to_endpoint(WebhookEndpoint& ep, const WebhookEvent& event) {
    try {
        Json::StreamWriterBuilder w;
        w["indentation"] = "";

        Json::Value payload;
        payload["event"] = event.type_name;
        payload["timestamp"] = static_cast<Json::Int64>(event.timestamp);
        payload["gateway"] = "hermes";
        payload["data"] = event.data;
        std::string body = Json::writeString(w, payload);

        size_t scheme_end = ep.url.find("://");
        if (scheme_end == std::string::npos) return false;

        std::string scheme = ep.url.substr(0, scheme_end);
        std::string rest = ep.url.substr(scheme_end + 3);

        size_t path_start = rest.find('/');
        std::string host_port = (path_start != std::string::npos) ? rest.substr(0, path_start) : rest;
        std::string path = (path_start != std::string::npos) ? rest.substr(path_start) : "/";

        std::string host = host_port;
        int port = (scheme == "https") ? 443 : 80;
        auto colon = host_port.find(':');
        if (colon != std::string::npos) {
            host = host_port.substr(0, colon);
            port = std::stoi(host_port.substr(colon + 1));
        }

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        if (scheme == "https") {
            httplib::SSLClient cli(host, port);
            cli.set_connection_timeout(ep.timeout_ms / 1000, (ep.timeout_ms % 1000) * 1000);
            cli.set_read_timeout(ep.timeout_ms / 1000, (ep.timeout_ms % 1000) * 1000);
            httplib::Headers hdrs;
            hdrs.emplace("Content-Type", "application/json");
            for (const auto& [k, v] : ep.headers) hdrs.emplace(k, v);
            auto result = cli.Post(path, hdrs, body, "application/json");
            return result && result->status >= 200 && result->status < 300;
        }
#endif
        httplib::Client cli(host, port);
        cli.set_connection_timeout(ep.timeout_ms / 1000, (ep.timeout_ms % 1000) * 1000);
        cli.set_read_timeout(ep.timeout_ms / 1000, (ep.timeout_ms % 1000) * 1000);
        httplib::Headers hdrs;
        hdrs.emplace("Content-Type", "application/json");
        for (const auto& [k, v] : ep.headers) hdrs.emplace(k, v);
        auto result = cli.Post(path, hdrs, body, "application/json");
        return result && result->status >= 200 && result->status < 300;
    } catch (...) {
        return false;
    }
}

std::string WebhookPlugin::mask_url(const std::string& url) {
    auto scheme_end = url.find("://");
    if (scheme_end == std::string::npos) return "***";

    auto path_start = url.find('/', scheme_end + 3);
    if (path_start == std::string::npos) return url.substr(0, scheme_end + 3) + "***";

    std::string host = url.substr(0, path_start);
    std::string path = url.substr(path_start);
    if (path.size() > 10) {
        return host + path.substr(0, 8) + "***";
    }
    return host + "/***";
}

std::string WebhookPlugin::event_type_to_string(WebhookEventType type) {
    switch (type) {
        case WebhookEventType::BackendDown: return "backend_down";
        case WebhookEventType::BackendUp: return "backend_up";
        case WebhookEventType::RateLimitHit: return "rate_limit_hit";
        case WebhookEventType::QuotaWarning: return "quota_warning";
        case WebhookEventType::QuotaExceeded: return "quota_exceeded";
        case WebhookEventType::KeyCreated: return "key_created";
        case WebhookEventType::KeyRevoked: return "key_revoked";
        case WebhookEventType::AuthFailure: return "auth_failure";
        case WebhookEventType::HighLatency: return "high_latency";
        case WebhookEventType::ErrorSpike: return "error_spike";
        case WebhookEventType::Custom: return "custom";
    }
    return "unknown";
}
