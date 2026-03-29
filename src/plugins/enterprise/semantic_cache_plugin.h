#pragma once
#include "../plugin.h"
#include "../ollama_client.h"
#include <shared_mutex>
#include <atomic>

class SemanticCachePlugin : public Plugin {
public:
    std::string name() const override { return "semantic_cache"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override;
    void shutdown() override;

    PluginResult before_request(
        Json::Value& body, PluginRequestContext& ctx) override;

    PluginResult after_response(
        Json::Value& response, PluginRequestContext& ctx) override;

    void set_ollama(OllamaClient* ollama) { ollama_ = ollama; }
    void set_ollama_backends(const std::vector<std::pair<std::string, int>>& b) {
        ollama_backends_ = b;
    }

    Json::Value stats() const override;

private:
    struct CacheEntry {
        std::vector<float> embedding;
        std::string response_json;
        std::string model;
        int64_t created_at = 0;
        int64_t last_hit = 0;
        int hit_count = 0;
    };

    mutable std::shared_mutex mtx_;
    std::vector<CacheEntry> entries_;
    std::unique_ptr<OllamaClient> own_ollama_;
    OllamaClient* ollama_ = nullptr;
    std::vector<std::pair<std::string, int>> ollama_backends_;

    float threshold_ = 0.95f;
    size_t max_entries_ = 5000;
    int ttl_seconds_ = 3600;
    std::string embedding_model_ = "nomic-embed-text";
    bool only_temperature_zero_ = true;
    std::vector<std::string> excluded_models_;

    std::atomic<int64_t> hits_{0};
    std::atomic<int64_t> misses_{0};

    [[nodiscard]] OllamaClient* get_ollama();
    [[nodiscard]] std::vector<float> get_embedding(const std::string& text,
        const std::string& model) const;
    [[nodiscard]] float cosine_similarity(const std::vector<float>& a,
        const std::vector<float>& b) const noexcept;
    void evict_expired();
    void evict_lru();
    [[nodiscard]] static std::string messages_to_prompt(const Json::Value& messages);
};
