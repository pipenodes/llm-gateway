#include "audit_plugin.h"
#include "../logger.h"
#include <json/json.h>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <iomanip>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

namespace {

std::string auditEntryToJsonLine(const core::AuditEntry& e) {
    Json::Value v;
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

    Json::StreamWriterBuilder w;
    w["indentation"] = "";
    return Json::writeString(w, v);
}

// RFC 5424: <PRI>VERSION TIMESTAMP HOSTNAME APP-NAME PROCID MSGID - MSG
std::string formatSyslogRFC5424(const core::AuditEntry& e, const std::string& app_name,
                               int facility, int severity, const std::string& json_msg) {
    int pri = facility * 8 + severity;
    if (pri > 191) pri = 191;
    if (pri < 0) pri = 0;

    auto ts = std::chrono::system_clock::from_time_t(e.timestamp / 1000);
    auto ms = e.timestamp % 1000;
    std::time_t t = std::chrono::system_clock::to_time_t(ts);
    std::tm tm_buf;
#ifdef _WIN32
    gmtime_s(&tm_buf, &t);
#else
    gmtime_r(&t, &tm_buf);
#endif
    std::ostringstream ts_ss;
    ts_ss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S");
    ts_ss << '.' << std::setfill('0') << std::setw(3) << ms << "Z";

    std::string escaped;
    escaped.reserve(json_msg.size() + 8);
    for (char c : json_msg) {
        if (c == '\\') escaped += "\\\\";
        else if (c == ']') escaped += "\\]";
        else escaped += c;
    }
    return "<" + std::to_string(pri) + ">1 " + ts_ss.str() + " - " + app_name + " - - - " + escaped;
}

} // namespace

AuditPlugin::~AuditPlugin() {
    shutdown();
}

bool AuditPlugin::init(const Json::Value& config) {
    output_path_ = config.get("path", "audit/").asString();
    output_ = config.get("output", "file").asString();
    queue_max_entries_ = static_cast<size_t>(config.get("queue_max_entries", 10000).asUInt());
    if (queue_max_entries_ == 0) queue_max_entries_ = 10000;

    syslog_host_ = config.get("syslog_host", "").asString();
    syslog_port_ = config.get("syslog_port", 514).asInt();
    if (syslog_port_ <= 0 || syslog_port_ > 65535) syslog_port_ = 514;
    syslog_protocol_ = config.get("syslog_protocol", "udp").asString();
    if (syslog_protocol_ != "tcp") syslog_protocol_ = "udp";
    syslog_facility_ = config.get("syslog_facility", 16).asInt();
    syslog_severity_ = config.get("syslog_severity", 6).asInt();
    syslog_app_name_ = config.get("syslog_app_name", "hermes").asString();
    if (syslog_app_name_.empty()) syslog_app_name_ = "hermes";

    if (!output_path_.empty() && output_path_.back() != '/')
        output_path_ += '/';

    if (!output_path_.empty())
        std::filesystem::create_directories(output_path_);

    worker_ = std::jthread([this] { worker_loop(); });
    return true;
}

void AuditPlugin::shutdown() {
    running_ = false;
    cv_.notify_all();
}

PluginResult AuditPlugin::before_request(
    Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult AuditPlugin::after_response(
    Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}

void AuditPlugin::on_request_completed(const core::AuditEntry& entry) {
    std::lock_guard lock(mtx_);
    if (queue_.size() >= queue_max_entries_)
        return;
    queue_.push(entry);
    cv_.notify_one();
}

void AuditPlugin::worker_loop() {
    while (running_) {
        core::AuditEntry entry;
        {
            std::unique_lock lock(mtx_);
            cv_.wait(lock, [this] {
                return !running_ || !queue_.empty();
            });
            if (!running_ && queue_.empty())
                break;
            if (queue_.empty())
                continue;
            entry = std::move(queue_.front());
            queue_.pop();
        }

        std::string line = auditEntryToJsonLine(entry);

        if (output_ == "syslog" && !syslog_host_.empty()) {
            std::string syslog_msg = formatSyslogRFC5424(entry, syslog_app_name_,
                                                        syslog_facility_, syslog_severity_, line);
            send_syslog(syslog_msg);
        }

        if (output_ != "syslog" && !output_path_.empty()) {
            std::string path = output_path_ + "audit.jsonl";
            std::ofstream ofs(path, std::ios::app);
            if (ofs) {
                ofs << line << '\n';
            } else {
                Json::Value f;
                f["request_id"] = entry.request_id;
                f["path"] = path;
                Logger::error("audit_write_failed", f);
            }
        }
    }
}

void AuditPlugin::send_syslog(const std::string& message) const {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return;
#endif
    struct addrinfo hints = {}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = (syslog_protocol_ == "tcp") ? SOCK_STREAM : SOCK_DGRAM;
    hints.ai_protocol = (syslog_protocol_ == "tcp") ? IPPROTO_TCP : IPPROTO_UDP;
    int gai = getaddrinfo(syslog_host_.c_str(), std::to_string(syslog_port_).c_str(), &hints, &res);
    if (gai != 0 || !res) {
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }
#ifdef _WIN32
    SOCKET sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == INVALID_SOCKET) {
        freeaddrinfo(res);
        WSACleanup();
        return;
    }
#else
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        freeaddrinfo(res);
        return;
    }
#endif
    if (syslog_protocol_ == "tcp") {
        if (connect(sock, res->ai_addr, static_cast<socklen_t>(res->ai_addrlen)) != 0) {
#ifdef _WIN32
            closesocket(sock);
#else
            close(sock);
#endif
            freeaddrinfo(res);
#ifdef _WIN32
            WSACleanup();
#endif
            return;
        }
    }
#ifdef _WIN32
    send(sock, message.data(), static_cast<int>(message.size()), 0);
    closesocket(sock);
#else
    send(sock, message.data(), message.size(), 0);
    close(sock);
#endif
#ifdef _WIN32
    WSACleanup();
#endif
    freeaddrinfo(res);
}

Json::Value AuditPlugin::query(const core::AuditQuery& q) const {
    Json::Value result(Json::objectValue);
    Json::Value entries(Json::arrayValue);
    int total = 0;
    int skipped = 0;

    std::string path = output_path_ + "audit.jsonl";
    std::ifstream ifs(path);
    if (!ifs)
        return result;

    std::string line;
    Json::CharReaderBuilder builder;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        Json::Value v;
        std::string errs;
        auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
        if (!reader->parse(line.data(), line.data() + line.size(), &v, &errs))
            continue;

        int64_t ts = v.get("timestamp", 0).asInt64();
        if (q.from_timestamp > 0 && ts < q.from_timestamp) continue;
        if (q.to_timestamp > 0 && ts > q.to_timestamp) continue;
        if (!q.key_alias.empty() && v.get("key_alias", "").asString() != q.key_alias) continue;
        if (!q.model.empty() && v.get("model", "").asString() != q.model) continue;
        if (q.status_code != 0 && v.get("status_code", 0).asInt() != q.status_code) continue;

        total++;
        if (skipped < q.offset) {
            skipped++;
            continue;
        }
        if (static_cast<int>(entries.size()) >= q.limit)
            continue;
        entries.append(v);
    }

    result["entries"] = std::move(entries);
    result["total"] = total;
    result["limit"] = q.limit;
    result["offset"] = q.offset;
    return result;
}
