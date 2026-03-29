/**
 * E2E tests for HERMES with all plugins active.
 * Requires: gateway running with config that enables all plugins (e.g. config.e2e.json).
 * Env: GATEWAY_URL (default http://localhost:8080), ADMIN_KEY (optional; admin tests skip if unset).
 */
#include <gtest/gtest.h>
#include <cstdlib>
#include <string>
#include <sstream>
#include <set>
#include "httplib.h"
#include <json/json.h>

namespace {

std::string getGatewayUrl() {
    const char* env = std::getenv("GATEWAY_URL");
    return env && env[0] ? std::string(env) : "http://localhost:8080";
}

std::string getAdminKey() {
    const char* env = std::getenv("ADMIN_KEY");
    return env ? std::string(env) : "";
}

bool parseJson(const std::string& body, Json::Value& root, std::string& errs) {
    Json::CharReaderBuilder builder;
    auto reader = builder.newCharReader();
    return reader->parse(body.data(), body.data() + body.size(), &root, &errs);
}

httplib::Client makeClient(const std::string& baseUrl) {
    httplib::Client cli(baseUrl);
    cli.set_connection_timeout(5, 0);
    cli.set_read_timeout(30, 0);
    return cli;
}

} // namespace

// -----------------------------------------------------------------------------
// Basic
// -----------------------------------------------------------------------------

TEST(E2E_Basic, Health) {
    std::string url = getGatewayUrl();
    auto cli = makeClient(url);
    auto res = cli.Get("/health");
    ASSERT_TRUE(res) << "Connection failed to " << url;
    EXPECT_EQ(res->status, 200);
    Json::Value root;
    std::string errs;
    ASSERT_TRUE(parseJson(res->body, root, errs)) << errs;
    EXPECT_TRUE(root.isMember("status"));
    EXPECT_EQ(root["status"].asString(), "ok");
}

TEST(E2E_Basic, AdminPluginsList) {
    std::string key = getAdminKey();
    if (key.empty()) {
        GTEST_SKIP() << "ADMIN_KEY not set; skip admin plugins test";
    }
    std::string url = getGatewayUrl();
    auto cli = makeClient(url);
    httplib::Headers headers{{"Authorization", "Bearer " + key}};
    auto res = cli.Get("/admin/plugins", headers);
    ASSERT_TRUE(res) << "Connection failed to " << url;
    if (res->status == 403)
        GTEST_SKIP() << "Admin returned 403 (wrong ADMIN_KEY or IP not in whitelist)";
    EXPECT_EQ(res->status, 200);
    Json::Value root;
    std::string errs;
    ASSERT_TRUE(parseJson(res->body, root, errs)) << errs;
    ASSERT_TRUE(root.isMember("plugins")) << "response missing 'plugins'";
    const auto& plugins = root["plugins"];
    ASSERT_TRUE(plugins.isArray());
    EXPECT_GE(plugins.size(), 1u) << "expected at least one plugin";
    std::set<std::string> names;
    for (Json::ArrayIndex i = 0; i < plugins.size(); ++i) {
        if (plugins[i].isMember("name"))
            names.insert(plugins[i]["name"].asString());
    }
    // Full pipeline check only when config has 15+ plugins (e.g. config.e2e.json)
    const std::set<std::string> expected_pipeline = {
        "logging", "semantic_cache", "pii_redactor", "content_moderation",
        "prompt_injection", "response_validator", "rag_injector", "cost_controller",
        "request_router", "conversation_memory", "adaptive_rate_limiter",
        "streaming_transformer", "api_versioning", "request_dedup", "model_warmup"
    };
    if (plugins.size() >= 15u) {
        for (const auto& e : expected_pipeline) {
            EXPECT_TRUE(names.count(e)) << "plugin '" << e << "' not found in list";
        }
    }
}

TEST(E2E_Basic, MetricsPluginsKey) {
    std::string url = getGatewayUrl();
    auto cli = makeClient(url);
    auto res = cli.Get("/metrics");
    ASSERT_TRUE(res) << "Connection failed to " << url;
    EXPECT_EQ(res->status, 200);
    Json::Value root;
    std::string errs;
    ASSERT_TRUE(parseJson(res->body, root, errs)) << errs;
    EXPECT_TRUE(root.isMember("plugins")) << "metrics response should have 'plugins' key";
}

TEST(E2E_Basic, GetModels) {
    std::string url = getGatewayUrl();
    auto cli = makeClient(url);
    auto res = cli.Get("/v1/models");
    ASSERT_TRUE(res) << "Connection failed to " << url;
    EXPECT_EQ(res->status, 200);
    Json::Value root;
    std::string errs;
    ASSERT_TRUE(parseJson(res->body, root, errs)) << errs;
    EXPECT_TRUE(root.isMember("data") || root.isMember("object") || root.isMember("created"))
        << "models response should have data/object/created";
}

// -----------------------------------------------------------------------------
// Intermediate
// -----------------------------------------------------------------------------

TEST(E2E_Intermediate, ChatCompletions) {
    std::string url = getGatewayUrl();
    auto cli = makeClient(url);
    std::string body = R"({
        "model": "gemma3:1b",
        "messages": [{"role": "user", "content": "Say hello in one word."}],
        "stream": false,
        "max_tokens": 10
    })";
    auto res = cli.Post("/v1/chat/completions", body, "application/json");
    if (!res) {
        GTEST_SKIP() << "Connection failed to " << url;
    }
    if (res->status == 404 || res->status == 502 || res->status == 503) {
        GTEST_SKIP() << "Model or backend unavailable (status " << res->status << ")";
    }
    EXPECT_EQ(res->status, 200) << res->body;
    Json::Value root;
    std::string errs;
    ASSERT_TRUE(parseJson(res->body, root, errs)) << errs;
    EXPECT_TRUE(root.isMember("choices") && root["choices"].isArray() && !root["choices"].empty());
}

TEST(E2E_Intermediate, Completions) {
    std::string url = getGatewayUrl();
    auto cli = makeClient(url);
    std::string body = R"({
        "model": "gemma3:1b",
        "prompt": "Once upon a time",
        "max_tokens": 5,
        "stream": false
    })";
    auto res = cli.Post("/v1/completions", body, "application/json");
    if (!res) {
        GTEST_SKIP() << "Connection failed to " << url;
    }
    if (res->status == 404 || res->status == 502 || res->status == 503) {
        GTEST_SKIP() << "Model or backend unavailable (status " << res->status << ")";
    }
    EXPECT_EQ(res->status, 200) << res->body;
    Json::Value root;
    std::string errs;
    ASSERT_TRUE(parseJson(res->body, root, errs)) << errs;
    EXPECT_TRUE(root.isMember("choices") || root.isMember("text_completion"));
}

TEST(E2E_Intermediate, Embeddings) {
    std::string url = getGatewayUrl();
    auto cli = makeClient(url);
    std::string body = R"({
        "model": "nomic-embed-text",
        "input": "test"
    })";
    auto res = cli.Post("/v1/embeddings", body, "application/json");
    if (!res) {
        GTEST_SKIP() << "Connection failed to " << url;
    }
    if (res->status == 404 || res->status == 503) {
        GTEST_SKIP() << "Embedding model or backend unavailable (status " << res->status << ")";
    }
    EXPECT_EQ(res->status, 200) << res->body;
    Json::Value root;
    std::string errs;
    ASSERT_TRUE(parseJson(res->body, root, errs)) << errs;
    EXPECT_TRUE(root.isMember("data") && root["data"].isArray());
}

// -----------------------------------------------------------------------------
// Advanced
// -----------------------------------------------------------------------------

TEST(E2E_Advanced, ChatStreaming) {
    std::string url = getGatewayUrl();
    auto cli = makeClient(url);
    std::string body = R"({
        "model": "gemma3:1b",
        "messages": [{"role": "user", "content": "Hi"}],
        "stream": true,
        "max_tokens": 5
    })";
    auto res = cli.Post("/v1/chat/completions", body, "application/json");
    if (!res) {
        GTEST_SKIP() << "Connection failed to " << url;
    }
    if (res->status == 404 || res->status == 503) {
        GTEST_SKIP() << "Model or backend unavailable (status " << res->status << ")";
    }
    EXPECT_EQ(res->status, 200);
    std::string ct = res->get_header_value("Content-Type");
    EXPECT_TRUE(ct.find("text/event-stream") != std::string::npos ||
                ct.find("event-stream") != std::string::npos)
        << "Content-Type should be event-stream, got: " << ct;
    EXPECT_TRUE(res->body.find("data:") != std::string::npos) << "response should contain SSE data lines";
}

TEST(E2E_Advanced, CacheHeaders) {
    std::string url = getGatewayUrl();
    auto cli = makeClient(url);
    std::string body = R"({
        "model": "gemma3:1b",
        "messages": [{"role": "user", "content": "E2E cache test unique"}],
        "stream": false,
        "temperature": 0,
        "max_tokens": 5
    })";
    auto res1 = cli.Post("/v1/chat/completions", body, "application/json");
    if (!res1 || res1->status != 200) {
        GTEST_SKIP() << "First request failed or backend unavailable";
    }
    auto res2 = cli.Post("/v1/chat/completions", body, "application/json");
    ASSERT_TRUE(res2);
    EXPECT_EQ(res2->status, 200);
    std::string xcache = res2->get_header_value("X-Cache");
    if (!xcache.empty()) {
        EXPECT_TRUE(xcache == "HIT" || xcache == "MISS" || xcache == "BYPASS")
            << "X-Cache should be HIT/MISS/BYPASS, got: " << xcache;
    }
}

TEST(E2E_Advanced, AdminKeysAndUpdatesCheck) {
    std::string key = getAdminKey();
    if (key.empty()) {
        GTEST_SKIP() << "ADMIN_KEY not set; skip admin keys/updates test";
    }
    std::string url = getGatewayUrl();
    auto cli = makeClient(url);
    httplib::Headers headers{{"Authorization", "Bearer " + key}};
    auto resKeys = cli.Get("/admin/keys", headers);
    ASSERT_TRUE(resKeys) << "Connection failed to " << url;
    if (resKeys->status == 403)
        GTEST_SKIP() << "Admin returned 403 (wrong ADMIN_KEY or IP not in whitelist)";
    EXPECT_EQ(resKeys->status, 200);
    auto resUpdates = cli.Get("/admin/updates/check", headers);
    ASSERT_TRUE(resUpdates) << "Connection failed to " << url;
    if (resUpdates->status == 403)
        GTEST_SKIP() << "Admin returned 403 (wrong ADMIN_KEY or IP not in whitelist)";
    EXPECT_EQ(resUpdates->status, 200);
}

TEST(E2E_Advanced, ApiVersioning) {
    std::string url = getGatewayUrl();
    auto cli = makeClient(url);
    httplib::Headers headers{{"X-Api-Version", "v2"}};
    auto res = cli.Get("/health", headers);
    ASSERT_TRUE(res) << "Connection failed to " << url;
    EXPECT_EQ(res->status, 200);
    if (res->has_header("X-Api-Version")) {
        EXPECT_EQ(res->get_header_value("X-Api-Version"), "v2");
    }
}
