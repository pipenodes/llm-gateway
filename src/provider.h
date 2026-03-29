#pragma once
#include <string>
#include <memory>
#include "httplib.h"
#include <json/json.h>
#include "chunk_queue.h"

struct RequestContext {
    std::string request_id;
    std::string key_alias;
    std::string client_ip;
};

class Provider {
public:
    virtual ~Provider() = default;

    [[nodiscard]] virtual std::string name() const = 0;

    [[nodiscard]] virtual bool supports_model(const std::string& model) const = 0;

    /** Returns true if this provider expects OpenAI-format request body (no conversion). */
    [[nodiscard]] virtual bool uses_openai_format() const { return false; }

    [[nodiscard]] virtual std::string default_model() const { return {}; }

    [[nodiscard]] virtual httplib::Result chat_completion(
        const std::string& body, const RequestContext& ctx) = 0;

    [[nodiscard]] virtual httplib::Result embeddings(
        const std::string& body, const RequestContext& ctx) = 0;

    [[nodiscard]] virtual httplib::Result list_models(const RequestContext& ctx) = 0;

    virtual void chat_completion_stream(
        const std::string& body_str,
        const std::string& chat_id,
        Json::Int64 created,
        const std::string& model,
        std::shared_ptr<ChunkQueue> queue,
        const RequestContext& ctx) = 0;

    [[nodiscard]] virtual bool is_healthy() const = 0;

    Provider(const Provider&) = delete;
    Provider& operator=(const Provider&) = delete;

protected:
    Provider() = default;
};
