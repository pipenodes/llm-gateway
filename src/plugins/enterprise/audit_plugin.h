#pragma once
#include "plugin.h"
#include "../core_services.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

class AuditPlugin : public Plugin, public core::IAuditSink, public core::IAuditQueryProvider {
public:
    AuditPlugin() = default;
    ~AuditPlugin() override;

    std::string name() const override { return "audit"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    void shutdown() override;

    PluginResult before_request(
        Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(
        Json::Value& response, PluginRequestContext& ctx) override;

    void on_request_completed(const core::AuditEntry& entry) override;

    [[nodiscard]] Json::Value query(const core::AuditQuery& q) const override;

private:
    void worker_loop();
    void send_syslog(const std::string& message) const;

    std::string output_path_;
    std::string output_{"file"};
    std::string syslog_host_;
    int syslog_port_ = 514;
    std::string syslog_protocol_{"udp"};
    int syslog_facility_ = 16;
    int syslog_severity_ = 6;
    std::string syslog_app_name_{"hermes"};
    size_t queue_max_entries_ = 10000;
    std::queue<core::AuditEntry> queue_;
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> running_{true};
    std::jthread worker_;
};
