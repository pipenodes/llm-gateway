#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>
#include <memory>
#include <unordered_map>
#include "httplib.h"
#include <json/json.h>
#include "logger.h"
#include "chunk_queue.h"
#include "crypto.h"

class OllamaClient {
public:
    struct Backend {
        std::string host;
        int port;
    };

    static constexpr int CONN_TIMEOUT = 5;
    static constexpr int CHAT_TIMEOUT = 300;
    static constexpr int SHORT_TIMEOUT = 10;
    static constexpr int MAX_RETRIES = 3;
    static constexpr int BASE_RETRY_MS = 200;
    static constexpr int HEALTH_INTERVAL_SECS = 30;
    static constexpr int ID_RANDOM_LENGTH = 24;
    static constexpr std::string_view ID_CHARSET = "abcdefghijklmnopqrstuvwxyz0123456789";

    OllamaClient() = default;
    OllamaClient(const OllamaClient&) = delete;
    OllamaClient& operator=(const OllamaClient&) = delete;
    ~OllamaClient();

    void init(const std::vector<std::pair<std::string, int>>& endpoints);

    [[nodiscard]] Backend selectBackend() const;

    [[nodiscard]] httplib::Result request(const std::string& method, const std::string& path,
                                          const std::string& body, int read_timeout);

    [[nodiscard]] httplib::Result chatCompletion(const std::string& body) {
        return request("POST", "/api/chat", body, CHAT_TIMEOUT);
    }

    [[nodiscard]] httplib::Result listModels() {
        return request("GET", "/api/tags", "", SHORT_TIMEOUT);
    }

    [[nodiscard]] httplib::Result embeddings(const std::string& body) {
        return request("POST", "/api/embed", body, SHORT_TIMEOUT);
    }

    [[nodiscard]] httplib::Result textCompletion(const std::string& body) {
        return request("POST", "/api/generate", body, CHAT_TIMEOUT);
    }

    [[nodiscard]] httplib::Result showModel(const std::string& model_name);

    void chatCompletionStream(const std::string& body_str,
                               const std::string& chat_id,
                               Json::Int64 created,
                               const std::string& model,
                               std::shared_ptr<ChunkQueue> queue);

    void textCompletionStream(const std::string& body_str,
                               const std::string& completion_id,
                               Json::Int64 created,
                               const std::string& model,
                               bool echo,
                               const std::string& prompt,
                               std::shared_ptr<ChunkQueue> queue);

    static Json::Value formatToolCallsStatic(const Json::Value& ollama_tool_calls);

private:
    std::vector<Backend> backends;
    std::vector<bool> backend_healthy;
    mutable std::mutex health_mtx;
    mutable std::atomic<uint64_t> rr_counter{0};
    std::thread health_thread;
    std::atomic<bool> running{true};

    struct ClientPool {
        std::unordered_map<std::string, std::unique_ptr<httplib::Client>> clients;
    };
    httplib::Client& getOrCreateClient(const Backend& backend);

    struct StreamThread {
        std::jthread thread;
        std::shared_ptr<std::atomic<bool>> done;
    };
    std::mutex stream_mtx;
    std::vector<StreamThread> stream_threads;

    void addStreamThread(std::jthread&& t, std::shared_ptr<std::atomic<bool>> done);
    void cleanupStreamThreads(bool join_all);
    void startHealthCheck();
};
