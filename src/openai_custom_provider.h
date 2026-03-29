#pragma once
#include "provider.h"
#include <string>
#include <memory>

class OpenAICustomProvider : public Provider {
public:
    struct Config {
        std::string id;
        std::string base_url;
        std::string api_key_env;
        std::string default_model;
    };

    explicit OpenAICustomProvider(Config config);

    [[nodiscard]] std::string name() const override { return config_.id; }
    [[nodiscard]] bool supports_model(const std::string&) const override { return true; }
    [[nodiscard]] bool uses_openai_format() const override { return true; }
    [[nodiscard]] std::string default_model() const override { return config_.default_model; }

    [[nodiscard]] httplib::Result chat_completion(
        const std::string& body, const RequestContext& ctx) override;
    [[nodiscard]] httplib::Result embeddings(
        const std::string& body, const RequestContext& ctx) override;
    [[nodiscard]] httplib::Result list_models(const RequestContext& ctx) override;

    void chat_completion_stream(
        const std::string& body_str,
        const std::string& chat_id,
        Json::Int64 created,
        const std::string& model,
        std::shared_ptr<ChunkQueue> queue,
        const RequestContext& ctx) override;

    [[nodiscard]] bool is_healthy() const override;

private:
    Config config_;
    std::string host_;
    int port_ = 443;
    std::string path_prefix_;
    bool use_ssl_ = true;
    std::string api_key_;

    void parse_base_url();
    std::string get_api_key() const;
};
