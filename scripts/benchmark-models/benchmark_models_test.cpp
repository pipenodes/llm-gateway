/**
 * Benchmark suite: compare performance of gemma3:1b, gemma3:4b, gemma3:12b
 * via Ollama (through HERMES) across subjects and prompt complexity.
 *
 * Env: GATEWAY_URL (default http://localhost:8080), API_KEY (optional).
 * Requires: gateway + Ollama running; models gemma3:1b, gemma3:4b, gemma3:12b pulled.
 */
#include <gtest/gtest.h>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "httplib.h"
#include <json/json.h>
#include "prompts.h"

namespace {

std::string getGatewayUrl() {
    const char* env = std::getenv("GATEWAY_URL");
    return env && env[0] ? std::string(env) : "http://localhost:8080";
}

std::string getApiKey() {
    const char* env = std::getenv("API_KEY");
    return env ? std::string(env) : "";
}

bool parseJson(const std::string& body, Json::Value& root, std::string& errs) {
    Json::CharReaderBuilder builder;
    auto reader = builder.newCharReader();
    return reader->parse(body.data(), body.data() + body.size(), &root, &errs);
}

struct RunResult {
    bool ok = false;
    int status = 0;
    double latency_ms = 0;
    int output_tokens = 0;
};

RunResult runChat(const std::string& baseUrl, const std::string& apiKey,
                  const std::string& model, const std::string& userMessage,
                  int maxTokens = 256) {
    RunResult r;
    httplib::Client cli(baseUrl);
    cli.set_connection_timeout(10, 0);
    cli.set_read_timeout(120, 0);

    Json::Value body;
    body["model"] = model;
    body["stream"] = false;
    body["max_tokens"] = maxTokens;
    body["messages"] = Json::Value(Json::arrayValue);
    body["messages"].append(Json::Value(Json::objectValue));
    body["messages"][0]["role"] = "user";
    body["messages"][0]["content"] = userMessage;

    Json::StreamWriterBuilder wbuilder;
    wbuilder["indentation"] = "";
    std::string bodyStr = Json::writeString(wbuilder, body);

    httplib::Headers headers;
    if (!apiKey.empty())
        headers.emplace("Authorization", "Bearer " + apiKey);

    auto start = std::chrono::steady_clock::now();
    auto res = cli.Post("/v1/chat/completions", headers, bodyStr, "application/json");
    auto end = std::chrono::steady_clock::now();
    r.latency_ms = std::chrono::duration<double, std::milli>(end - start).count();

    if (!res) {
        r.ok = false;
        r.status = -1;
        return r;
    }
    r.status = res->status;
    if (res->status != 200) {
        r.ok = false;
        return r;
    }

    Json::Value root;
    std::string errs;
    if (!parseJson(res->body, root, errs)) {
        r.ok = false;
        return r;
    }
    if (root.isMember("choices") && root["choices"].isArray() && !root["choices"].empty()) {
        const auto& choice = root["choices"][0];
        if (choice.isMember("message") && choice["message"].isMember("content"))
            r.output_tokens = static_cast<int>(choice["message"]["content"].asString().size() / 4);
        if (root.isMember("usage") && root["usage"].isMember("completion_tokens"))
            r.output_tokens = root["usage"]["completion_tokens"].asInt();
    }
    r.ok = true;
    return r;
}

// Models and subject/complexity names for reporting
const std::vector<std::string> MODELS = { "gemma3:1b", "gemma3:4b", "gemma3:12b" };
const std::vector<std::string> SUBJECT_NAMES = {
    "general", "math", "logical_reasoning", "python", "javascript", "csharp"
};
const std::vector<std::string> COMPLEXITY_NAMES = { "basic", "medium", "advanced" };

} // namespace

class BenchmarkModels : public ::testing::Test {
protected:
    void SetUp() override {
        url_ = getGatewayUrl();
        apiKey_ = getApiKey();
        prompts_ = benchmark::getAllPrompts();
        ASSERT_EQ(prompts_.size(), 6u) << "expected 6 subjects";
        for (const auto& s : prompts_)
            ASSERT_EQ(s.size(), 3u) << "expected 3 complexity levels per subject";
    }

    std::string url_;
    std::string apiKey_;
    std::vector<std::vector<std::vector<std::string>>> prompts_;
};

TEST_F(BenchmarkModels, RunFullBenchmark) {
    if (url_.empty()) {
        GTEST_SKIP() << "GATEWAY_URL not set";
    }

    // [model_idx][subject][complexity] -> { total_ms, count, failures }
    struct Stats { double total_ms = 0; int count = 0; int failures = 0; };
    std::vector<std::vector<std::vector<Stats>>> stats(
        MODELS.size(),
        std::vector<std::vector<Stats>>(6, std::vector<Stats>(3))
    );

    std::cout << "\n=== Benchmark: gemma3:1b vs gemma3:4b vs gemma3:12b (Ollama) ===\n";
    std::cout << "Gateway: " << url_ << "  Questions: 10 per (subject, complexity)\n\n";

    for (size_t m = 0; m < MODELS.size(); ++m) {
        const std::string& model = MODELS[m];
        std::cout << "Model: " << model << "\n";
        for (size_t subj = 0; subj < 6u; ++subj) {
            for (size_t comp = 0; comp < 3u; ++comp) {
                const auto& questions = prompts_[subj][comp];
                for (size_t q = 0; q < questions.size(); ++q) {
                    RunResult r = runChat(url_, apiKey_, model, questions[q]);
                    stats[m][subj][comp].total_ms += r.latency_ms;
                    stats[m][subj][comp].count++;
                    if (!r.ok)
                        stats[m][subj][comp].failures++;
                }
            }
        }
    }

    // Summary table: per model, per subject, average latency
    std::cout << "\n--- Average latency (ms) per subject (all complexities) ---\n";
    std::cout << std::fixed << std::setprecision(1);
    for (const auto& subjName : SUBJECT_NAMES)
        std::cout << "\t" << subjName;
    std::cout << "\n";

    for (size_t m = 0; m < MODELS.size(); ++m) {
        std::cout << MODELS[m];
        for (size_t subj = 0; subj < 6u; ++subj) {
            double total = 0;
            int cnt = 0;
            for (size_t comp = 0; comp < 3u; ++comp) {
                total += stats[m][subj][comp].total_ms;
                cnt += stats[m][subj][comp].count;
            }
            double avg = (cnt > 0) ? (total / cnt) : 0;
            std::cout << "\t" << avg;
        }
        std::cout << "\n";
    }

    std::cout << "\n--- Average latency (ms) per (subject, complexity) for each model ---\n";
    for (size_t subj = 0; subj < 6u; ++subj) {
        for (size_t comp = 0; comp < 3u; ++comp) {
            std::cout << SUBJECT_NAMES[subj] << "/" << COMPLEXITY_NAMES[comp] << ": ";
            for (size_t m = 0; m < MODELS.size(); ++m) {
                const auto& s = stats[m][subj][comp];
                double avg = (s.count > 0) ? (s.total_ms / s.count) : 0;
                std::cout << MODELS[m] << "=" << avg << "ms";
                if (s.failures > 0) std::cout << "(" << s.failures << " fail)";
                if (m + 1 < MODELS.size()) std::cout << "  ";
            }
            std::cout << "\n";
        }
    }

    // Failures: skip only if gateway unreachable; otherwise record
    int totalFailures = 0;
    for (size_t m = 0; m < MODELS.size(); ++m)
        for (size_t subj = 0; subj < 6u; ++subj)
            for (size_t comp = 0; comp < 3u; ++comp)
                totalFailures += stats[m][subj][comp].failures;

    if (totalFailures > 0) {
        std::cout << "\nTotal failed requests: " << totalFailures << "\n";
        EXPECT_EQ(totalFailures, 0) << "Some requests failed (check gateway and Ollama models)";
    }
    std::cout << "\n=== Benchmark complete ===\n";
}

// Optional: per-model test for quicker iteration (one model only)
class BenchmarkOneModel : public ::testing::TestWithParam<std::string> {};

TEST_P(BenchmarkOneModel, OneModelAllCategories) {
    std::string url = getGatewayUrl();
    if (url.empty()) {
        GTEST_SKIP() << "GATEWAY_URL not set";
    }
    std::string model = GetParam();
    auto prompts = benchmark::getAllPrompts();
    int runs = 0, fails = 0;
    double totalMs = 0;
    for (size_t subj = 0; subj < prompts.size(); ++subj) {
        for (size_t comp = 0; comp < prompts[subj].size(); ++comp) {
            for (const auto& q : prompts[subj][comp]) {
                RunResult r = runChat(url, getApiKey(), model, q);
                totalMs += r.latency_ms;
                runs++;
                if (!r.ok) fails++;
            }
        }
    }
    std::cout << "Model " << model << ": " << runs << " runs, " << fails << " failures, "
              << std::fixed << std::setprecision(1) << (runs ? totalMs / runs : 0) << " ms avg\n";
    EXPECT_EQ(fails, 0) << "Model " << model << " had " << fails << " failed requests";
}

INSTANTIATE_TEST_SUITE_P(Models, BenchmarkOneModel,
    ::testing::Values("gemma3:1b", "gemma3:4b", "gemma3:12b"));
