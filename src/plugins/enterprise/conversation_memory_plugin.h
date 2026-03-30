#pragma once
#include "plugin.h"
#include <unordered_map>
#include <deque>
#include <shared_mutex>
#include <string>
#include <atomic>
#include <thread>
#include <cstdint>

struct Session {
    std::string id;
    std::string key_alias;
    std::deque<Json::Value> messages;
    int64_t created_at = 0;
    int64_t last_activity = 0;
    int total_chars = 0;
};

class ConversationMemoryPlugin : public Plugin {
public:
    ConversationMemoryPlugin() = default;
    ~ConversationMemoryPlugin() override;

    std::string name() const override { return "conversation_memory"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    void shutdown() override;
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, PluginRequestContext& ctx) override;
    Json::Value stats() const override;

    [[nodiscard]] Json::Value list_sessions() const;
    [[nodiscard]] Json::Value get_session(const std::string& session_id) const;
    bool delete_session(const std::string& session_id);

private:
    [[nodiscard]] std::string resolve_session_id(const PluginRequestContext& ctx) const;
    void cleanup_expired();
    void cleanup_loop();
    void trim_session(Session& s) const;
    void load_sessions();
    void save_sessions() const;

    std::unordered_map<std::string, Session> sessions_;
    mutable std::shared_mutex mtx_;

    size_t max_messages_ = 50;
    size_t max_chars_ = 8000;
    int session_ttl_seconds_ = 3600;
    std::string session_header_ = "X-Session-Id";
    std::string persist_path_;

    std::atomic<bool> running_{false};
    std::atomic<bool> dirty_{false};
    std::jthread cleanup_thread_;
};
