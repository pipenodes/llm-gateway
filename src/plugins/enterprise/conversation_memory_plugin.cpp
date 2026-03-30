#include "conversation_memory_plugin.h"
#include "logger.h"
#include "crypto.h"
#include <fstream>

ConversationMemoryPlugin::~ConversationMemoryPlugin() {
    shutdown();
}

bool ConversationMemoryPlugin::init(const Json::Value& config) {
    max_messages_ = config.get("max_messages", 50).asUInt();
    max_chars_ = config.get("max_chars", 8000).asUInt();
    session_ttl_seconds_ = config.get("session_ttl_seconds", 3600).asInt();
    session_header_ = config.get("session_header", "X-Session-Id").asString();
    persist_path_ = config.get("persist_path", "").asString();

    if (!persist_path_.empty())
        load_sessions();

    running_ = true;
    cleanup_thread_ = std::jthread([this] { cleanup_loop(); });

    Logger::info("conversation_memory_init");
    return true;
}

void ConversationMemoryPlugin::shutdown() {
    if (running_.exchange(false)) {
        if (cleanup_thread_.joinable()) cleanup_thread_.join();
        if (!persist_path_.empty()) {
            std::shared_lock lock(mtx_);
            save_sessions();
        }
    }
}

std::string ConversationMemoryPlugin::resolve_session_id(const PluginRequestContext& ctx) const {
    auto it = ctx.metadata.find("session_id");
    if (it != ctx.metadata.end() && !it->second.empty())
        return it->second;
    return ctx.key_alias.empty()
        ? "anon:" + ctx.client_ip
        : ctx.key_alias + ":" + ctx.client_ip;
}

PluginResult ConversationMemoryPlugin::before_request(Json::Value& body, PluginRequestContext& ctx) {
    if (!body.isMember("messages") || !body["messages"].isArray())
        return PluginResult{.action = PluginAction::Continue};

    std::string sid = resolve_session_id(ctx);
    int64_t now = static_cast<int64_t>(crypto::epoch_seconds());

    Json::Value user_messages(Json::arrayValue);
    auto& messages = body["messages"];
    for (Json::ArrayIndex i = 0; i < messages.size(); ++i) {
        if (messages[i].get("role", "").asString() == "user")
            user_messages.append(messages[i]);
    }

    std::unique_lock lock(mtx_);
    auto it = sessions_.find(sid);
    if (it != sessions_.end()) {
        auto& session = it->second;
        session.last_activity = now;

        Json::Value system_msgs(Json::arrayValue);
        Json::Value new_user_msgs(Json::arrayValue);
        for (Json::ArrayIndex i = 0; i < messages.size(); ++i) {
            std::string role = messages[i].get("role", "").asString();
            if (role == "system")
                system_msgs.append(messages[i]);
            else if (role == "user")
                new_user_msgs.append(messages[i]);
        }

        Json::Value rebuilt(Json::arrayValue);
        for (Json::ArrayIndex i = 0; i < system_msgs.size(); ++i)
            rebuilt.append(system_msgs[i]);

        size_t injected_chars = 0;
        for (const auto& msg : session.messages) {
            size_t msg_chars = msg.get("content", "").asString().size();
            if (injected_chars + msg_chars > max_chars_) break;
            rebuilt.append(msg);
            injected_chars += msg_chars;
        }

        for (Json::ArrayIndex i = 0; i < new_user_msgs.size(); ++i)
            rebuilt.append(new_user_msgs[i]);

        body["messages"] = std::move(rebuilt);

        for (Json::ArrayIndex i = 0; i < user_messages.size(); ++i) {
            session.messages.push_back(user_messages[i]);
            session.total_chars += static_cast<int>(
                user_messages[i].get("content", "").asString().size());
        }
        trim_session(session);
    } else {
        Session session;
        session.id = sid;
        session.key_alias = ctx.key_alias;
        session.created_at = now;
        session.last_activity = now;
        session.total_chars = 0;
        for (Json::ArrayIndex i = 0; i < user_messages.size(); ++i) {
            session.messages.push_back(user_messages[i]);
            session.total_chars += static_cast<int>(
                user_messages[i].get("content", "").asString().size());
        }
        sessions_[sid] = std::move(session);
    }
    dirty_ = true;

    return PluginResult{.action = PluginAction::Continue};
}

PluginResult ConversationMemoryPlugin::after_response(Json::Value& response, PluginRequestContext& ctx) {
    if (!response.isMember("choices") || !response["choices"].isArray() || response["choices"].empty())
        return PluginResult{.action = PluginAction::Continue};

    std::string content = response["choices"][0]["message"].get("content", "").asString();
    if (content.empty())
        return PluginResult{.action = PluginAction::Continue};

    std::string sid = resolve_session_id(ctx);

    Json::Value assistant_msg;
    assistant_msg["role"] = "assistant";
    assistant_msg["content"] = content;

    std::unique_lock lock(mtx_);
    auto it = sessions_.find(sid);
    if (it != sessions_.end()) {
        it->second.messages.push_back(assistant_msg);
        it->second.total_chars += static_cast<int>(content.size());
        it->second.last_activity = static_cast<int64_t>(crypto::epoch_seconds());
        trim_session(it->second);
        dirty_ = true;
    }

    return PluginResult{.action = PluginAction::Continue};
}

Json::Value ConversationMemoryPlugin::stats() const {
    std::shared_lock lock(mtx_);
    Json::Value out;
    out["active_sessions"] = static_cast<int>(sessions_.size());
    out["max_messages"] = static_cast<int>(max_messages_);
    out["session_ttl_seconds"] = session_ttl_seconds_;
    return out;
}

Json::Value ConversationMemoryPlugin::list_sessions() const {
    std::shared_lock lock(mtx_);
    Json::Value out(Json::arrayValue);
    for (const auto& [id, s] : sessions_) {
        Json::Value entry;
        entry["id"] = s.id;
        entry["key_alias"] = s.key_alias;
        entry["messages"] = static_cast<int>(s.messages.size());
        entry["total_chars"] = s.total_chars;
        entry["created_at"] = static_cast<Json::Int64>(s.created_at);
        entry["last_activity"] = static_cast<Json::Int64>(s.last_activity);
        out.append(entry);
    }
    return out;
}

Json::Value ConversationMemoryPlugin::get_session(const std::string& session_id) const {
    std::shared_lock lock(mtx_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) return Json::Value();

    const auto& s = it->second;
    Json::Value out;
    out["id"] = s.id;
    out["key_alias"] = s.key_alias;
    out["messages_count"] = static_cast<int>(s.messages.size());
    out["total_chars"] = s.total_chars;
    out["created_at"] = static_cast<Json::Int64>(s.created_at);
    out["last_activity"] = static_cast<Json::Int64>(s.last_activity);

    Json::Value msgs(Json::arrayValue);
    for (const auto& m : s.messages) msgs.append(m);
    out["messages"] = msgs;
    return out;
}

bool ConversationMemoryPlugin::delete_session(const std::string& session_id) {
    std::unique_lock lock(mtx_);
    return sessions_.erase(session_id) > 0;
}

void ConversationMemoryPlugin::trim_session(Session& s) const {
    while (s.messages.size() > max_messages_) {
        s.total_chars -= static_cast<int>(
            s.messages.front().get("content", "").asString().size());
        s.messages.pop_front();
    }
}

void ConversationMemoryPlugin::cleanup_expired() {
    int64_t now = static_cast<int64_t>(crypto::epoch_seconds());
    std::unique_lock lock(mtx_);
    for (auto it = sessions_.begin(); it != sessions_.end();) {
        if (now - it->second.last_activity > session_ttl_seconds_)
            it = sessions_.erase(it);
        else
            ++it;
    }
}

void ConversationMemoryPlugin::cleanup_loop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(60));
        if (!running_) break;
        cleanup_expired();
        if (!persist_path_.empty() && dirty_.exchange(false)) {
            std::shared_lock lock(mtx_);
            save_sessions();
        }
    }
}

void ConversationMemoryPlugin::load_sessions() {
    if (persist_path_.empty()) return;
    std::ifstream file(persist_path_);
    if (!file.is_open()) return;

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    if (!Json::parseFromStream(builder, file, &root, &errs) || !root.isArray()) return;

    std::unique_lock lock(mtx_);
    int64_t now = static_cast<int64_t>(crypto::epoch_seconds());

    for (Json::ArrayIndex i = 0; i < root.size(); ++i) {
        const auto& s = root[i];
        Session session;
        session.id = s.get("id", "").asString();
        session.key_alias = s.get("key_alias", "").asString();
        session.created_at = s.get("created_at", 0).asInt64();
        session.last_activity = s.get("last_activity", 0).asInt64();
        session.total_chars = s.get("total_chars", 0).asInt();

        if (now - session.last_activity > session_ttl_seconds_) continue;

        if (s.isMember("messages") && s["messages"].isArray()) {
            for (const auto& m : s["messages"])
                session.messages.push_back(m);
        }
        sessions_[session.id] = std::move(session);
    }

    Json::Value f;
    f["loaded_sessions"] = static_cast<int>(sessions_.size());
    Logger::info("sessions_loaded", f);
}

void ConversationMemoryPlugin::save_sessions() const {
    if (persist_path_.empty()) return;

    Json::StreamWriterBuilder w;
    w["indentation"] = "  ";
    Json::Value root(Json::arrayValue);

    for (const auto& [id, s] : sessions_) {
        Json::Value entry;
        entry["id"] = s.id;
        entry["key_alias"] = s.key_alias;
        entry["created_at"] = static_cast<Json::Int64>(s.created_at);
        entry["last_activity"] = static_cast<Json::Int64>(s.last_activity);
        entry["total_chars"] = s.total_chars;

        Json::Value msgs(Json::arrayValue);
        for (const auto& m : s.messages) msgs.append(m);
        entry["messages"] = msgs;
        root.append(entry);
    }

    std::ofstream file(persist_path_);
    if (file.is_open())
        file << Json::writeString(w, root) << '\n';
}
