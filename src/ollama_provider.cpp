#include "ollama_provider.h"

OllamaProvider::OllamaProvider(std::string default_model, std::string provider_name)
    : default_model_(std::move(default_model)), name_(std::move(provider_name)) {}

void OllamaProvider::init(const std::vector<std::pair<std::string, int>>& endpoints) {
    ollama_.init(endpoints);
}

bool OllamaProvider::supports_model(const std::string&) const {
    return true;
}

httplib::Result OllamaProvider::chat_completion(
    const std::string& body, const RequestContext&) {
    return ollama_.chatCompletion(body);
}

httplib::Result OllamaProvider::embeddings(
    const std::string& body, const RequestContext&) {
    return ollama_.embeddings(body);
}

httplib::Result OllamaProvider::list_models(const RequestContext&) {
    return ollama_.listModels();
}

void OllamaProvider::chat_completion_stream(
    const std::string& body_str,
    const std::string& chat_id,
    Json::Int64 created,
    const std::string& model,
    std::shared_ptr<ChunkQueue> queue,
    const RequestContext&) {
    ollama_.chatCompletionStream(body_str, chat_id, created, model, queue);
}

void OllamaProvider::text_completion_stream(
    const std::string& body_str,
    const std::string& completion_id,
    Json::Int64 created,
    const std::string& model,
    bool echo,
    const std::string& prompt,
    std::shared_ptr<ChunkQueue> queue,
    const RequestContext&) {
    ollama_.textCompletionStream(body_str, completion_id, created, model, echo, prompt, queue);
}

httplib::Result OllamaProvider::text_completion(const std::string& body, const RequestContext&) {
    return ollama_.textCompletion(body);
}

httplib::Result OllamaProvider::show_model(const std::string& model_name) {
    return ollama_.showModel(model_name);
}

bool OllamaProvider::is_healthy() const {
    auto backend = ollama_.selectBackend();
    return !backend.host.empty();
}
