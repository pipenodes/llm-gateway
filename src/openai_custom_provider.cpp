#include "openai_custom_provider.h"
#include "logger.h"
#include <cstdlib>
#include <cstring>
#include <charconv>
#include <sstream>

namespace {

void parse_url(const std::string& url, std::string& scheme, std::string& host,
               int& port, std::string& path) {
    scheme = "https";
    host.clear();
    port = 443;
    path = "/v1";

    std::string rest = url;
    if (rest.starts_with("https://")) {
        scheme = "https";
        rest = rest.substr(8);
    } else if (rest.starts_with("http://")) {
        scheme = "http";
        port = 80;
        rest = rest.substr(7);
    }

    auto path_start = rest.find('/');
    if (path_start != std::string::npos) {
        host = rest.substr(0, path_start);
        path = rest.substr(path_start);
        if (!path.ends_with('/'))
            path += '/';
    } else {
        host = rest;
        path = "/v1/";
    }

    auto colon = host.find(':');
    if (colon != std::string::npos) {
        port = 0;
        auto [ptr, ec] = std::from_chars(host.data() + colon + 1,
            host.data() + host.size(), port);
        if (ec == std::errc{} && port > 0) {
            host = host.substr(0, colon);
        } else {
            port = (scheme == "https") ? 443 : 80;
        }
    }
}

} // namespace

OpenAICustomProvider::OpenAICustomProvider(Config config)
    : config_(std::move(config)) {
    parse_base_url();
    api_key_ = get_api_key();
}

void OpenAICustomProvider::parse_base_url() {
    std::string scheme;
    parse_url(config_.base_url, scheme, host_, port_, path_prefix_);
    use_ssl_ = (scheme == "https");
    if (!path_prefix_.ends_with('/'))
        path_prefix_ += '/';
}

std::string OpenAICustomProvider::get_api_key() const {
    if (!config_.api_key_env.empty()) {
        if (const char* v = std::getenv(config_.api_key_env.c_str()))
            return v;
    }
    return {};
}

httplib::Result OpenAICustomProvider::chat_completion(
    const std::string& body, const RequestContext&) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    if (!use_ssl_) {
        httplib::Client cli(host_, port_);
        cli.set_connection_timeout(5, 0);
        cli.set_read_timeout(300, 0);
        auto path = path_prefix_ + "chat/completions";
        httplib::Headers headers;
        if (!api_key_.empty())
            headers.emplace("Authorization", "Bearer " + api_key_);
        return cli.Post(path, body, "application/json");
    }

    httplib::SSLClient cli(host_, port_);
    cli.set_connection_timeout(5, 0);
    cli.set_read_timeout(300, 0);
    auto path = path_prefix_ + "chat/completions";
    httplib::Headers headers;
    if (!api_key_.empty())
        headers.emplace("Authorization", "Bearer " + api_key_);
    return cli.Post(path, headers, body, "application/json");
#else
    (void)body;
    return std::nullopt;
#endif
}

httplib::Result OpenAICustomProvider::embeddings(
    const std::string& body, const RequestContext&) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    if (!use_ssl_) {
        httplib::Client cli(host_, port_);
        cli.set_connection_timeout(5, 0);
        cli.set_read_timeout(30, 0);
        auto path = path_prefix_ + "embeddings";
        httplib::Headers headers;
        if (!api_key_.empty())
            headers.emplace("Authorization", "Bearer " + api_key_);
        return cli.Post(path, body, "application/json");
    }

    httplib::SSLClient cli(host_, port_);
    cli.set_connection_timeout(5, 0);
    cli.set_read_timeout(30, 0);
    auto path = path_prefix_ + "embeddings";
    httplib::Headers headers;
    if (!api_key_.empty())
        headers.emplace("Authorization", "Bearer " + api_key_);
    return cli.Post(path, headers, body, "application/json");
#else
    (void)body;
    return std::nullopt;
#endif
}

httplib::Result OpenAICustomProvider::list_models(const RequestContext&) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    if (!use_ssl_) {
        httplib::Client cli(host_, port_);
        cli.set_connection_timeout(5, 0);
        cli.set_read_timeout(10, 0);
        auto path = path_prefix_ + "models";
        httplib::Headers headers;
        if (!api_key_.empty())
            headers.emplace("Authorization", "Bearer " + api_key_);
        return cli.Get(path);
    }

    httplib::SSLClient cli(host_, port_);
    cli.set_connection_timeout(5, 0);
    cli.set_read_timeout(10, 0);
    auto path = path_prefix_ + "models";
    httplib::Headers headers;
    if (!api_key_.empty())
        headers.emplace("Authorization", "Bearer " + api_key_);
    return cli.Get(path);
#else
    return std::nullopt;
#endif
}

void OpenAICustomProvider::chat_completion_stream(
    const std::string& body_str,
    const std::string& /* chat_id */,
    Json::Int64 /* created */,
    const std::string& /* model */,
    std::shared_ptr<ChunkQueue> queue,
    const RequestContext&) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    auto path = path_prefix_ + "chat/completions";
    httplib::Headers headers;
    if (!api_key_.empty())
        headers.emplace("Authorization", "Bearer " + api_key_);

    auto receiver = [queue](const char* data, size_t len) -> bool {
        if (queue->cancelled.load(std::memory_order_acquire)) return false;
        queue->push(std::string(data, len));
        return true;
    };

    if (use_ssl_) {
        httplib::SSLClient cli(host_, port_);
        cli.set_connection_timeout(5, 0);
        cli.set_read_timeout(300, 0);
        auto res = cli.Post(path, headers, body_str, "application/json", receiver, nullptr);
        if (!res) {
            queue->push("data: {\"error\":{\"message\":\"Connection failed\"}}\n\n");
        }
    } else {
        httplib::Client cli(host_, port_);
        cli.set_connection_timeout(5, 0);
        cli.set_read_timeout(300, 0);
        auto res = cli.Post(path, headers, body_str, "application/json", receiver, nullptr);
        if (!res) {
            queue->push("data: {\"error\":{\"message\":\"Connection failed\"}}\n\n");
        }
    }
    queue->push("data: [DONE]\n\n");
    queue->finish();
#else
    (void)body_str;
    (void)chat_id;
    (void)created;
    (void)model;
    queue->push("data: {\"error\":{\"message\":\"HTTPS not supported (OpenSSL required)\"}}\n\n");
    queue->push("data: [DONE]\n\n");
    queue->finish();
#endif
}

bool OpenAICustomProvider::is_healthy() const {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    if (!use_ssl_) {
        httplib::Client cli(host_, port_);
        cli.set_connection_timeout(2, 0);
        auto path = path_prefix_ + "models";
        httplib::Headers headers;
        if (!api_key_.empty())
            headers.emplace("Authorization", "Bearer " + api_key_);
        auto res = cli.Get(path);
        return res && res->status == 200;
    }
    httplib::SSLClient cli(host_, port_);
    cli.set_connection_timeout(2, 0);
    auto path = path_prefix_ + "models";
    httplib::Headers headers;
    if (!api_key_.empty())
        headers.emplace("Authorization", "Bearer " + api_key_);
    auto res = cli.Get(path);
    return res && res->status == 200;
#else
    return false;
#endif
}
