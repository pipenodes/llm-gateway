/**
 * Full benchmark suite: metrics (TTFT, latency, tokens/s, resources), scoring (exact match, F1, BLEU, math, logic),
 * reports (report-xxxx.json, .csv, .txt) and gemma3:12b-generated report-xxxx.md.
 */
#include <gtest/gtest.h>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "httplib.h"
#include <json/json.h>
#include "prompts.h"
#include "metrics/latency_meter.h"
#include "metrics/token_counter.h"
#include "metrics/resource_monitor.h"
#include "scoring/expected_answers.h"
#include "scoring/math_validator.h"
#include "scoring/logic_validator.h"
#include "scoring/score_utils.h"
#include <algorithm>
#include <map>

namespace {

std::string getGatewayUrl() {
    const char* e = std::getenv("GATEWAY_URL");
    return e && e[0] ? std::string(e) : "http://localhost:8080";
}
std::string getApiKey() {
    const char* e = std::getenv("API_KEY");
    return e ? std::string(e) : "";
}

bool parseJson(const std::string& body, Json::Value& root, std::string& errs) {
    Json::CharReaderBuilder b;
    auto r = b.newCharReader();
    return r->parse(body.data(), body.data() + body.size(), &root, &errs);
}

struct RunRecord {
    std::string model, subject, complexity;
    int q_idx = 0;
    std::string question, answer;
    double ttft_ms = -1.0, total_latency_ms = 0.0;
    int tokens_in = 0, tokens_out = 0;
    double tokens_per_sec = 0.0;
    double ram_mb = 0, vram_mb = 0, cpu_pct = 0, gpu_pct = 0;
    bool exact_match = false;
    double f1 = 0.0, bleu = 0.0;
    bool math_correct = false;
    double coherence = 0.0, step_by_step = 0.0, adheres = 0.0;
    bool ok = false;
};

// Non-streaming: get total latency and token counts.
RunRecord runChatNonStream(const std::string& baseUrl, const std::string& apiKey,
    const std::string& model, const std::string& userMessage, int maxTokens,
    const std::string& systemMessage = "") {
    RunRecord rec;
    rec.model = model;
    rec.question = userMessage;
    rec.ok = false;
    httplib::Client cli(baseUrl);
    cli.set_connection_timeout(10, 0);
    cli.set_read_timeout(120, 0);
    Json::Value body;
    body["model"] = model;
    body["stream"] = false;
    body["max_tokens"] = maxTokens;
    body["messages"] = Json::Value(Json::arrayValue);
    if (!systemMessage.empty()) {
        body["messages"].append(Json::Value(Json::objectValue));
        body["messages"][static_cast<Json::ArrayIndex>(body["messages"].size() - 1)]["role"] = "system";
        body["messages"][static_cast<Json::ArrayIndex>(body["messages"].size() - 1)]["content"] = systemMessage;
    }
    body["messages"].append(Json::Value(Json::objectValue));
    body["messages"][static_cast<Json::ArrayIndex>(body["messages"].size() - 1)]["role"] = "user";
    body["messages"][static_cast<Json::ArrayIndex>(body["messages"].size() - 1)]["content"] = userMessage;
    Json::StreamWriterBuilder wb;
    wb["indentation"] = "";
    std::string bodyStr = Json::writeString(wb, body);
    httplib::Headers headers;
    if (!apiKey.empty()) headers.emplace("Authorization", "Bearer " + apiKey);

    auto start = std::chrono::steady_clock::now();
    auto res = cli.Post("/v1/chat/completions", headers, bodyStr, "application/json");
    auto end = std::chrono::steady_clock::now();
    rec.total_latency_ms = std::chrono::duration<double, std::milli>(end - start).count();
    rec.ttft_ms = -1.0; // not measured without streaming

    if (!res || res->status != 200) return rec;
    Json::Value root;
    std::string errs;
    if (!parseJson(res->body, root, errs)) return rec;
    if (root.isMember("choices") && root["choices"].isArray() && !root["choices"].empty()) {
        const auto& c = root["choices"][0];
        if (c.isMember("message") && c["message"].isMember("content"))
            rec.answer = c["message"]["content"].asString();
    }
    if (root.isMember("usage")) {
        const auto& u = root["usage"];
        if (u.isMember("prompt_tokens")) rec.tokens_in = u["prompt_tokens"].asInt();
        if (u.isMember("completion_tokens")) rec.tokens_out = u["completion_tokens"].asInt();
    }
    if (rec.tokens_out == 0 && !rec.answer.empty())
        rec.tokens_out = static_cast<int>(rec.answer.size() / 4);
    rec.tokens_per_sec = (rec.total_latency_ms > 0 && rec.tokens_out > 0)
        ? (rec.tokens_out * 1000.0 / rec.total_latency_ms) : 0.0;
    rec.ok = true;
    return rec;
}

// Streaming: measure TTFT and collect full response.
RunRecord runChatStream(const std::string& baseUrl, const std::string& apiKey,
    const std::string& model, const std::string& userMessage, int maxTokens,
    const std::string& systemMessage = "") {
    RunRecord rec;
    rec.model = model;
    rec.question = userMessage;
    rec.ok = false;
    httplib::Client cli(baseUrl);
    cli.set_connection_timeout(10, 0);
    cli.set_read_timeout(120, 0);
    Json::Value body;
    body["model"] = model;
    body["stream"] = true;
    body["max_tokens"] = maxTokens;
    body["messages"] = Json::Value(Json::arrayValue);
    if (!systemMessage.empty()) {
        body["messages"].append(Json::Value(Json::objectValue));
        body["messages"][static_cast<Json::ArrayIndex>(body["messages"].size() - 1)]["role"] = "system";
        body["messages"][static_cast<Json::ArrayIndex>(body["messages"].size() - 1)]["content"] = systemMessage;
    }
    body["messages"].append(Json::Value(Json::objectValue));
    body["messages"][static_cast<Json::ArrayIndex>(body["messages"].size() - 1)]["role"] = "user";
    body["messages"][static_cast<Json::ArrayIndex>(body["messages"].size() - 1)]["content"] = userMessage;
    Json::StreamWriterBuilder wb;
    wb["indentation"] = "";
    std::string bodyStr = Json::writeString(wb, body);
    httplib::Headers headers;
    if (!apiKey.empty()) headers.emplace("Authorization", "Bearer " + apiKey);

    benchmark::LatencyMeter meter;
    std::string buffer;
    std::string fullContent;
    int usage_completion = 0, usage_prompt = 0;

    meter.start();
    auto res = cli.Post("/v1/chat/completions", headers, bodyStr, "application/json",
        [&](const char* data, size_t len) {
            buffer.append(data, len);
            for (;;) {
                size_t pos = buffer.find('\n');
                if (pos == std::string::npos) break;
                std::string line = buffer.substr(0, pos);
                buffer.erase(0, pos + 1);
                if (line.size() > 6 && line.compare(0, 6, "data: ") == 0) {
                    std::string jsonStr = line.substr(6);
                    if (jsonStr == "[DONE]" || jsonStr.empty()) continue;
                    Json::Value chunk;
                    std::string errs;
                    if (parseJson(jsonStr, chunk, errs)) {
                        if (chunk.isMember("choices") && chunk["choices"].isArray() && !chunk["choices"].empty()) {
                            const auto& ch = chunk["choices"][0];
                            if (ch.isMember("delta") && ch["delta"].isMember("content")) {
                                std::string delta = ch["delta"]["content"].asString();
                                if (!delta.empty() && fullContent.empty()) meter.record_first_token();
                                fullContent += delta;
                            }
                        }
                        if (chunk.isMember("usage")) {
                            if (chunk["usage"].isMember("completion_tokens"))
                                usage_completion = chunk["usage"]["completion_tokens"].asInt();
                            if (chunk["usage"].isMember("prompt_tokens"))
                                usage_prompt = chunk["usage"]["prompt_tokens"].asInt();
                        }
                    }
                }
            }
            return true;
        });
    meter.stop();

    rec.total_latency_ms = meter.total_ms();
    rec.ttft_ms = meter.ttft_ms();
    rec.answer = fullContent;
    rec.tokens_in = usage_prompt;
    rec.tokens_out = usage_completion > 0 ? usage_completion : static_cast<int>(fullContent.size() / 4);
    rec.tokens_per_sec = (rec.total_latency_ms > 0 && rec.tokens_out > 0)
        ? (rec.tokens_out * 1000.0 / rec.total_latency_ms) : 0.0;
    rec.ok = (res && res->status == 200);
    return rec;
}

std::string reportId() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm* tm = std::localtime(&t);
    std::ostringstream os;
    os << std::put_time(tm, "%Y%m%d-%H%M%S");
    return os.str();
}

std::string executionDate() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm* tm = std::localtime(&t);
    std::ostringstream os;
    os << std::put_time(tm, "%Y-%m-%dT%H:%M:%S");
    return os.str();
}

std::string modelsSlug(const std::vector<std::string>& models) {
    std::string s;
    for (size_t i = 0; i < models.size(); ++i) {
        if (i) s += "_";
        for (char c : models[i]) s += (c == ':') ? '_' : c;
    }
    return s.empty() ? "default" : s;
}

std::string getReportDir() {
    const char* e = std::getenv("REPORT_DIR");
    return (e && e[0]) ? std::string(e) : ".";
}

bool loadBenchmarkConfig(const std::string& outDir, std::string& provider, std::vector<std::string>& models,
    std::string& defaultSystemPrompt, std::map<std::string, std::string>& systemPromptsBySubject) {
    std::string path = outDir + "/benchmark_config.json";
    std::ifstream f(path);
    if (!f) return false;
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();
    Json::Value root;
    std::string errs;
    if (!parseJson(content, root, errs)) return false;
    if (root.isMember("provider")) provider = root["provider"].asString();
    else provider = "ollama";
    models.clear();
    if (root.isMember("models") && root["models"].isArray()) {
        for (const auto& m : root["models"]) models.push_back(m.asString());
    }
    defaultSystemPrompt.clear();
    systemPromptsBySubject.clear();
    if (root.isMember("system_prompt") && root["system_prompt"].isString())
        defaultSystemPrompt = root["system_prompt"].asString();
    if (root.isMember("system_prompts") && root["system_prompts"].isObject()) {
        for (const auto& key : root["system_prompts"].getMemberNames())
            if (root["system_prompts"][key].isString())
                systemPromptsBySubject[key] = root["system_prompts"][key].asString();
    }
    return !models.empty();
}

void writeReports(const std::string& id, const std::string& execDate, const std::string& provider, const std::vector<std::string>& models, const std::vector<RunRecord>& runs,
    const std::string& jsonPath, const std::string& csvPath, const std::string& txtPath,
    const std::string& outDir) {
    Json::Value root;
    root["report_id"] = id;
    root["execution_date"] = execDate;
    root["provider"] = provider;
    root["models"] = Json::Value(Json::arrayValue);
    for (const auto& m : models) root["models"].append(m);
    root["runs"] = Json::Value(Json::arrayValue);

    double sumLatency = 0, sumTtft = 0;
    int totalOk = 0, totalCorrect = 0;
    for (const auto& r : runs) {
        if (r.ok) { totalOk++; sumLatency += r.total_latency_ms; if (r.ttft_ms >= 0) sumTtft += r.ttft_ms; }
        if (r.exact_match || r.math_correct) totalCorrect++;
    }
    Json::Value summary;
    summary["total_runs"] = static_cast<int>(runs.size());
    summary["ok_count"] = totalOk;
    summary["correct_count"] = totalCorrect;
    summary["avg_latency_ms"] = (totalOk > 0) ? (sumLatency / totalOk) : 0.0;
    summary["avg_ttft_ms"] = (totalOk > 0 && sumTtft >= 0) ? (sumTtft / totalOk) : -1.0;
    summary["cost_per_correct"] = (totalCorrect > 0) ? (sumLatency / 1000.0 / totalCorrect) : 0.0; // seconds per correct
    root["summary"] = summary;

    for (const auto& r : runs) {
        Json::Value j;
        j["model"] = r.model;
        j["subject"] = r.subject;
        j["complexity"] = r.complexity;
        j["q_idx"] = r.q_idx;
        j["question"] = r.question;
        j["answer"] = r.answer;
        j["ttft_ms"] = r.ttft_ms;
        j["total_latency_ms"] = r.total_latency_ms;
        j["tokens_in"] = r.tokens_in;
        j["tokens_out"] = r.tokens_out;
        j["tokens_per_sec"] = r.tokens_per_sec;
        j["exact_match"] = r.exact_match;
        j["f1"] = r.f1;
        j["bleu"] = r.bleu;
        j["math_correct"] = r.math_correct;
        j["coherence"] = r.coherence;
        j["step_by_step"] = r.step_by_step;
        j["adheres"] = r.adheres;
        j["ok"] = r.ok;
        root["runs"].append(j);
    }
    std::ofstream fj(jsonPath);
    Json::StreamWriterBuilder wb;
    wb["indentation"] = "  ";
    fj << Json::writeString(wb, root);
    fj.close();

    std::ofstream fc(csvPath);
    fc << "# report_id: " << id << "\n";
    fc << "# execution_date: " << execDate << "\n";
    fc << "# provider: " << provider << "\n";
    fc << "# models: ";
    for (size_t i = 0; i < models.size(); ++i) fc << (i ? "," : "") << models[i];
    fc << "\nmodel,subject,complexity,q_idx,ttft_ms,total_latency_ms,tokens_in,tokens_out,tokens_per_sec,exact_match,f1,bleu,math_correct,coherence,step_by_step,adheres,ok\n";
    for (const auto& r : runs) {
        fc << r.model << "," << r.subject << "," << r.complexity << "," << r.q_idx << ","
           << r.ttft_ms << "," << r.total_latency_ms << "," << r.tokens_in << "," << r.tokens_out << ","
           << r.tokens_per_sec << "," << (r.exact_match ? "1" : "0") << "," << r.f1 << "," << r.bleu << ","
           << (r.math_correct ? "1" : "0") << "," << r.coherence << "," << r.step_by_step << "," << r.adheres << ","
           << (r.ok ? "1" : "0") << "\n";
    }
    fc.close();

    std::ofstream ft(txtPath);
    ft << "report_id: " << id << "\nexecution_date: " << execDate << "\nprovider: " << provider << "\nModels: ";
    for (size_t i = 0; i < models.size(); ++i) ft << (i ? ", " : "") << models[i];
    ft << "\n\n";
    std::string lastQ;
    for (const auto& r : runs) {
        if (r.question != lastQ) {
            if (!lastQ.empty()) ft << "\n";
            ft << "--- QUESTION ---\n" << r.question << "\n\n";
            lastQ = r.question;
        }
        ft << "Model: " << r.model << "\nAnswer: " << r.answer << "\n\n";
    }
    ft.close();
    // Also write canonical name (latest run) in same dir
    std::string latestTxt = outDir + "/report_questions_and_answers.txt";
    std::ifstream in(txtPath);
    std::ofstream ftLatest(latestTxt);
    ftLatest << in.rdbuf();
    in.close();
    ftLatest.close();
}

bool generateAnalysisMd(const std::string& baseUrl, const std::string& apiKey,
    const std::string& jsonPath, const std::string& csvPath, const std::string& txtPath,
    const std::string& mdPath, const std::string& reportId, const std::string& execDate, const std::string& provider, const std::vector<std::string>& models) {
    std::ifstream fj(jsonPath);
    std::ifstream fc(csvPath);
    std::ifstream ft(txtPath);
    std::string jsonContent((std::istreambuf_iterator<char>(fj)), std::istreambuf_iterator<char>());
    std::string csvContent((std::istreambuf_iterator<char>(fc)), std::istreambuf_iterator<char>());
    std::string txtContent((std::istreambuf_iterator<char>(ft)), std::istreambuf_iterator<char>());
    fj.close(); fc.close(); ft.close();

    std::string modelsStr;
    for (size_t i = 0; i < models.size(); ++i) modelsStr += (i ? ", " : "") + models[i];
    std::string prompt = "You are an expert analyst. This benchmark report uses the following model(s): " + modelsStr + ". "
        "Based on the following benchmark data (JSON summary, CSV rows, and Q&A text), write a critical qualitative and quantitative analysis in Markdown. "
        "Focus on: (1) TTFT, total latency, tokens/s, and output quality for the model(s). "
        "(2) Objective metrics: exact match, F1, BLEU, math correctness. (3) Subjective/cognitive: structural coherence, step-by-step reasoning, adherence. "
        "(4) IEC (Eficiência Cognitiva = quality/cost). (5) Scalability: does the 12B deliver proportionally better value? (6) Cost per correct answer. "
        "Be concise but insightful. Use tables or bullet points where appropriate.\n\n";
    prompt += "=== JSON (excerpt) ===\n" + jsonContent.substr(0, 15000) + "\n\n";
    prompt += "=== CSV (first 50 lines) ===\n";
    std::istringstream csvStream(csvContent);
    std::string line; int l = 0;
    while (l < 50 && std::getline(csvStream, line)) { prompt += line + "\n"; ++l; }
    prompt += "\n=== Q&A (excerpt) ===\n" + txtContent.substr(0, 8000) + "\n";

    httplib::Client cli(baseUrl);
    cli.set_connection_timeout(15, 0);
    cli.set_read_timeout(180, 0);
    Json::Value body;
    body["model"] = "gemma3:12b";
    body["stream"] = false;
    body["max_tokens"] = 4096;
    body["messages"] = Json::Value(Json::arrayValue);
    body["messages"].append(Json::Value(Json::objectValue));
    body["messages"][0]["role"] = "user";
    body["messages"][0]["content"] = prompt;
    Json::StreamWriterBuilder wb;
    wb["indentation"] = "";
    std::string bodyStr = Json::writeString(wb, body);
    httplib::Headers headers;
    if (!apiKey.empty()) headers.emplace("Authorization", "Bearer " + apiKey);

    auto res = cli.Post("/v1/chat/completions", headers, bodyStr, "application/json");
    if (!res || res->status != 200) return false;
    Json::Value root;
    std::string errs;
    if (!parseJson(res->body, root, errs)) return false;
    std::string mdContent;
    if (root.isMember("choices") && root["choices"].isArray() && !root["choices"].empty()) {
        const auto& c = root["choices"][0];
        if (c.isMember("message") && c["message"].isMember("content"))
            mdContent = c["message"]["content"].asString();
    }
    std::ofstream fm(mdPath);
    fm << "report_id: " << reportId << "\nexecution_date: " << execDate << "\nprovider: " << provider << "\nModels: " << modelsStr << "\n\n";
    fm << mdContent;
    fm.close();
    return true;
}

static std::vector<std::string> getModelsFromEnv() {
    const char* e = std::getenv("BENCHMARK_MODELS");
    if (e && e[0]) {
        std::vector<std::string> out;
        std::string s(e);
        for (size_t i = 0; i < s.size(); ) {
            while (i < s.size() && (s[i] == ',' || s[i] == ' ')) ++i;
            size_t j = i;
            while (j < s.size() && s[j] != ',' && s[j] != ' ') ++j;
            if (j > i) out.push_back(s.substr(i, j - i));
            i = j;
        }
        if (!out.empty()) return out;
    }
    return { "gemma3:1b", "gemma3:4b", "gemma3:12b" };
}

const std::vector<std::string> SUBJECTS = { "general", "math", "logical_reasoning", "python", "javascript", "csharp" };
const std::vector<std::string> COMPLEXITIES = { "basic", "medium", "advanced" };

} // namespace

class BenchmarkSuite : public ::testing::Test {
protected:
    void SetUp() override {
        url_ = getGatewayUrl();
        apiKey_ = getApiKey();
        prompts_ = benchmark::getAllPrompts();
    }
    std::string url_;
    std::string apiKey_;
    std::vector<std::vector<std::vector<std::string>>> prompts_;
};

TEST_F(BenchmarkSuite, FullRunAndReports) {
    if (url_.empty()) { GTEST_SKIP() << "GATEWAY_URL not set"; }
    const std::string outDir = getReportDir();
    const std::string id = reportId();
    const std::string execDate = executionDate();
    std::string provider = "ollama";
    std::vector<std::string> models;
    std::string defaultSystemPrompt;
    std::map<std::string, std::string> systemPromptsBySubject;
    if (!loadBenchmarkConfig(outDir, provider, models, defaultSystemPrompt, systemPromptsBySubject))
        models = getModelsFromEnv();
    const std::string slug = modelsSlug(models);
    const std::string jsonPath = outDir + "/report-" + slug + "-" + id + ".json";
    const std::string csvPath = outDir + "/report-" + slug + "-" + id + ".csv";
    const std::string txtPath = outDir + "/report-" + slug + "-" + id + "_questions_and_answers.txt";
    const std::string mdPath = outDir + "/report-" + slug + "-" + id + ".md";
    std::vector<RunRecord> runs;
    const bool useStream = true; // set false to skip TTFT (faster)

    for (size_t mi = 0; mi < models.size(); ++mi) {
        const std::string& model = models[mi];
        for (size_t si = 0; si < prompts_.size(); ++si) {
            for (size_t ci = 0; ci < prompts_[si].size(); ++ci) {
                const auto& questions = prompts_[si][ci];
                for (size_t qi = 0; qi < questions.size(); ++qi) {
                    std::string systemMsg;
                    auto it = systemPromptsBySubject.find(SUBJECTS[si]);
                    if (it != systemPromptsBySubject.end()) systemMsg = it->second;
                    if (systemMsg.empty()) systemMsg = defaultSystemPrompt;
                    RunRecord r = useStream
                        ? runChatStream(url_, apiKey_, model, questions[qi], 256, systemMsg)
                        : runChatNonStream(url_, apiKey_, model, questions[qi], 256, systemMsg);
                    r.subject = SUBJECTS[si];
                    r.complexity = COMPLEXITIES[ci];
                    r.q_idx = static_cast<int>(qi);

                    benchmark::ExpectedAnswer exp = benchmark::getExpectedAnswer(static_cast<int>(si), static_cast<int>(ci), static_cast<int>(qi));
                    r.exact_match = benchmark::exactMatch(r.answer, exp);
                    r.math_correct = exp.has_expected && !exp.numbers.empty() && benchmark::mathCorrect(r.answer, exp);
                    if (exp.has_expected && !exp.exact.empty()) {
                        r.f1 = benchmark::f1Score(r.answer, exp.exact);
                        r.bleu = benchmark::simpleBleu(r.answer, exp.exact);
                    }
                    benchmark::LogicScores ls = benchmark::logicScores(r.answer, r.question);
                    r.coherence = ls.structural_coherence;
                    r.step_by_step = ls.step_by_step;
                    r.adheres = ls.adheres_to_constraint;

                    runs.push_back(r);
                }
            }
        }
    }

    writeReports(id, execDate, provider, models, runs, jsonPath, csvPath, txtPath, outDir);
    std::cout << "Wrote " << jsonPath << ", " << csvPath << ", " << txtPath << " (report_questions_and_answers)\n";

    bool mdOk = generateAnalysisMd(url_, apiKey_, jsonPath, csvPath, txtPath, mdPath, id, execDate, provider, models);
    if (mdOk) std::cout << "Wrote " << mdPath << " (analysis by gemma3:12b)\n";
    else std::cout << "Could not generate " << mdPath << " (check gemma3:12b availability)\n";

    // IEC and cost per correct (simplified)
    int totalOk = 0, totalCorrect = 0;
    for (const auto& r : runs) {
        if (r.ok) totalOk++;
        if (r.exact_match || r.math_correct) totalCorrect++;
    }
    std::cout << "Runs: " << runs.size() << " ok=" << totalOk << " correct=" << totalCorrect << "\n";
    EXPECT_GT(totalOk, 0u) << "At least one run should succeed";
}
