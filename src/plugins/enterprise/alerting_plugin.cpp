#include "alerting_plugin.h"
#include "logger.h"
#include "httplib.h"
#include <json/json.h>
#include <chrono>
#include <sstream>

namespace {

void auditEntryToJson(Json::Value& v, const core::AuditEntry& e) {
    v["timestamp"] = static_cast<Json::Int64>(e.timestamp);
    v["request_id"] = e.request_id;
    v["key_alias"] = e.key_alias;
    v["client_ip"] = e.client_ip;
    v["method"] = e.method;
    v["path"] = e.path;
    v["model"] = e.model;
    v["status_code"] = e.status_code;
    v["prompt_tokens"] = e.prompt_tokens;
    v["completion_tokens"] = e.completion_tokens;
    v["latency_ms"] = static_cast<Json::Int64>(e.latency_ms);
    v["stream"] = e.stream;
    v["cache_hit"] = e.cache_hit;
    if (!e.error.empty()) v["error"] = e.error;
}

bool parseWebhookUrl(const std::string& url, std::string& scheme, std::string& host, int& port, std::string& path) {
    if (url.empty()) return false;
    scheme = "https";
    host.clear();
    port = 443;
    path = "/";

    if (url.find("http://") == 0) {
        scheme = "http";
        port = 80;
    } else if (url.find("https://") != 0) {
        return false;
    }

    size_t start = (scheme == "https") ? 8u : 7u;
    size_t slash = url.find('/', start);
    std::string host_port = (slash == std::string::npos) ? url.substr(start) : url.substr(start, slash - start);
    if (slash != std::string::npos && slash < url.size())
        path = url.substr(slash);

    size_t colon = host_port.rfind(':');
    if (colon != std::string::npos && colon > 0) {
        host = host_port.substr(0, colon);
        try {
            port = std::stoi(host_port.substr(colon + 1));
        } catch (...) {
            return false;
        }
    } else {
        host = host_port;
    }
    return !host.empty();
}

} // namespace

AlertingPlugin::~AlertingPlugin() {
    shutdown();
}

bool AlertingPlugin::init(const Json::Value& config) {
    webhook_url_ = config.get("webhook_url", "").asString();
    if (webhook_url_.empty()) return false;

    window_seconds_ = config.get("window_seconds", 300).asInt();
    if (window_seconds_ <= 0) window_seconds_ = 300;
    status_4xx_limit_ = config.get("status_4xx_limit", 10).asInt();
    if (status_4xx_limit_ <= 0) status_4xx_limit_ = 10;
    alert_on_blocked_ = config.get("alert_on_blocked", true).asBool();

    worker_ = std::jthread([this] { worker_loop(); });
    return true;
}

void AlertingPlugin::shutdown() {
    running_ = false;
    cv_.notify_all();
}

PluginResult AlertingPlugin::before_request(
    Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult AlertingPlugin::after_response(
    Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}

void AlertingPlugin::on_request_completed(const core::AuditEntry& entry) {
    std::string payload;
    {
        std::lock_guard lock(mtx_);

        if (alert_on_blocked_ && entry.status_code >= 400 && entry.status_code < 500) {
            Json::Value body(Json::objectValue);
            body["event"] = "blocked";
            body["timestamp"] = static_cast<Json::Int64>(entry.timestamp);
            Json::Value entries(Json::arrayValue);
            Json::Value e;
            auditEntryToJson(e, entry);
            entries.append(e);
            body["entries"] = entries;
            body["rule"] = "any_4xx";
            Json::StreamWriterBuilder w;
            w["indentation"] = "";
            payload = Json::writeString(w, body);
            payload_queue_.push(payload);
            cv_.notify_one();
        }

        if (entry.status_code >= 400 && entry.status_code < 500) {
            std::string key = entry.key_alias.empty() ? "*" : entry.key_alias;
            int64_t now = entry.timestamp;
            auto& q = key_4xx_timestamps_[key];
            q.push_back(now);
            while (!q.empty() && (now - q.front()) > window_seconds_)
                q.pop_front();
            if (static_cast<int>(q.size()) >= status_4xx_limit_) {
                Json::Value body(Json::objectValue);
                body["event"] = "status_4xx";
                body["timestamp"] = static_cast<Json::Int64>(now);
                body["key_alias"] = key;
                body["count"] = static_cast<int>(q.size());
                body["window_seconds"] = window_seconds_;
                Json::Value entries(Json::arrayValue);
                Json::Value e;
                auditEntryToJson(e, entry);
                entries.append(e);
                body["entries"] = entries;
                Json::StreamWriterBuilder w;
                w["indentation"] = "";
                payload = Json::writeString(w, body);
                payload_queue_.push(payload);
                cv_.notify_one();
                q.clear();
            }
        }
    }
}

void AlertingPlugin::worker_loop() {
    while (running_) {
        std::string payload;
        {
            std::unique_lock lock(mtx_);
            cv_.wait(lock, [this] {
                return !running_ || !payload_queue_.empty();
            });
            if (!running_ && payload_queue_.empty()) break;
            if (payload_queue_.empty()) continue;
            payload = std::move(payload_queue_.front());
            payload_queue_.pop();
        }
        try_send_webhook(payload);
    }
}

void AlertingPlugin::try_send_webhook(const std::string& payload) {
    std::string scheme, host, path;
    int port = 0;
    if (!parseWebhookUrl(webhook_url_, scheme, host, port, path)) {
        Json::Value f;
        f["webhook_url"] = webhook_url_;
        Logger::error("alerting_invalid_webhook_url", f);
        return;
    }
    bool ok = false;
    if (scheme == "https") {
        httplib::SSLClient cli(host, port);
        cli.set_connection_timeout(5, 0);
        cli.set_read_timeout(5, 0);
        auto res = cli.Post(path.c_str(), payload, "application/json");
        ok = res && res->status >= 200 && res->status < 300;
    } else {
        httplib::Client cli(host, port);
        cli.set_connection_timeout(5, 0);
        cli.set_read_timeout(5, 0);
        auto res = cli.Post(path.c_str(), payload, "application/json");
        ok = res && res->status >= 200 && res->status < 300;
    }
    if (!ok) {
        Json::Value f;
        f["host"] = host;
        f["path"] = path;
        Logger::warn("alerting_webhook_failed", f);
    }
}

