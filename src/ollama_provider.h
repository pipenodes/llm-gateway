#pragma once
#include "provider.h"
#include "ollama_client.h"
#include <vector>
#include <string>

class OllamaProvider : public Provider {
public:
    explicit OllamaProvider(std::string default_model, std::string provider_name = "ollama");

    void init(const std::vector<std::pair<std::string, int>>& endpoints);

    [[nodiscard]] std::string name() const override { return name_; }
    [[nodiscard]] bool supports_model(const std::string& model) const override;
    [[nodiscard]] bool uses_openai_format() const override { return false; }
    [[nodiscard]] std::string default_model() const override { return default_model_; }

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

    void text_completion_stream(
        const std::string& body_str,
        const std::string& completion_id,
        Json::Int64 created,
        const std::string& model,
        bool echo,
        const std::string& prompt,
        std::shared_ptr<ChunkQueue> queue,
        const RequestContext& ctx);

    [[nodiscard]] httplib::Result text_completion(const std::string& body, const RequestContext& ctx);
    [[nodiscard]] httplib::Result show_model(const std::string& model_name);

    [[nodiscard]] bool is_healthy() const override;

private:
    OllamaClient ollama_;
    std::string default_model_;
    std::string name_;
};
