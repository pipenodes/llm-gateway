#include "benchmark.h"
#include <chrono>
#include <algorithm>
#include <sstream>

namespace {

std::string build_chat_body(const std::string& model, const std::string& prompt,
                            const Json::Value& params,
                            const Json::Value* tools = nullptr,
                            const Json::Value* tool_choice = nullptr) {
    Json::Value messages(Json::arrayValue);
    Json::Value msg;
    msg["role"] = "user";
    msg["content"] = prompt;
    messages.append(msg);

    Json::Value body;
    body["model"] = model;
    body["messages"] = messages;
    if (params.isObject()) {
        if (params.isMember("temperature")) body["temperature"] = params["temperature"];
        if (params.isMember("max_tokens")) body["max_tokens"] = params["max_tokens"];
    }
    body["stream"] = false;
    if (tools && tools->isArray() && tools->size() > 0) {
        body["tools"] = *tools;
    }
    if (tool_choice && !tool_choice->isNull()) {
        body["tool_choice"] = *tool_choice;
    }

    Json::StreamWriterBuilder wb;
    return Json::writeString(wb, body);
}

std::string build_eval_prompt(const std::string& original_prompt, const std::string& response) {
    return "Avalie a resposta abaixo em relacao ao prompt. Retorne APENAS um JSON valido com as chaves: score (0-100), relevance (0-100), coherence (0-100), conciseness (0-100), reason (string breve).\n\nPrompt: " + original_prompt + "\n\nResposta: " + response;
}

} // namespace

BenchmarkRunner::EvalResult BenchmarkRunner::parse_evaluation(const std::string& llm_response) {
    EvalResult r;
    // Try to extract JSON from response (may be wrapped in markdown)
    std::string s = llm_response;
    size_t start = s.find('{');
    size_t end = s.rfind('}');
    if (start != std::string::npos && end != std::string::npos && end > start) {
        s = s.substr(start, end - start + 1);
    }
    Json::CharReaderBuilder rb;
    std::istringstream iss(s);
    Json::Value j;
    if (Json::parseFromStream(rb, iss, &j, nullptr)) {
        auto safeInt = [](const Json::Value& v) -> int {
            if (v.isInt()) return v.asInt();
            if (v.isString()) { int x = 0; std::istringstream(v.asString()) >> x; return x; }
            return 0;
        };
        if (j.isMember("score")) r.score = safeInt(j["score"]);
        if (j.isMember("relevance")) r.relevance = safeInt(j["relevance"]);
        if (j.isMember("coherence")) r.coherence = safeInt(j["coherence"]);
        if (j.isMember("conciseness")) r.conciseness = safeInt(j["conciseness"]);
        if (j.isMember("reason") && j["reason"].isString()) r.reason = j["reason"].asString();
    }
    return r;
}

Json::Value BenchmarkRunner::run(const Json::Value& request, ChatFn chat_fn) {
    Json::Value out;
    out["status"] = "completed";

    if (!request.isMember("models") || !request["models"].isArray()) {
        out["status"] = "error";
        out["error"] = "models array required";
        return out;
    }
    if (!request.isMember("prompts") || !request["prompts"].isArray()) {
        out["status"] = "error";
        out["error"] = "prompts array required";
        return out;
    }
    if (!request.isMember("evaluator") || !request["evaluator"].isObject()) {
        out["status"] = "error";
        out["error"] = "evaluator object required";
        return out;
    }

    const auto& models = request["models"];
    const auto& prompts = request["prompts"];
    const auto& params = request.get("params", Json::objectValue);
    const auto& evaluator = request["evaluator"];
    const Json::Value* tools = request.isMember("tools") && request["tools"].isArray() ? &request["tools"] : nullptr;
    const Json::Value* tool_choice = request.isMember("tool_choice") ? &request["tool_choice"] : nullptr;
    std::string eval_model = evaluator.isMember("model") && evaluator["model"].isString()
        ? evaluator["model"].asString() : "";
    std::string eval_provider = evaluator.isMember("provider") && evaluator["provider"].isString()
        ? evaluator["provider"].asString() : "ollama";

    if (models.size() < 2 || models.size() > 5) {
        out["status"] = "error";
        out["error"] = "models must have 2-5 entries";
        return out;
    }
    if (prompts.size() < 1 || prompts.size() > 10) {
        out["status"] = "error";
        out["error"] = "prompts must have 1-10 entries";
        return out;
    }
    if (eval_model.empty()) {
        out["status"] = "error";
        out["error"] = "evaluator.model required";
        return out;
    }

    Json::Value results(Json::arrayValue);
    std::map<std::string, double> total_latency;
    std::map<std::string, double> total_tokens_per_sec;
    std::map<std::string, int> total_score;
    std::map<std::string, int> count;

    for (Json::ArrayIndex m = 0; m < models.size(); ++m) {
        std::string model;
        std::string provider = "ollama";
        if (models[m].isObject()) {
            if (models[m].isMember("model") && models[m]["model"].isString())
                model = models[m]["model"].asString();
            if (models[m].isMember("provider") && models[m]["provider"].isString())
                provider = models[m]["provider"].asString();
        } else if (models[m].isString()) {
            model = models[m].asString();
        }
        if (model.empty()) continue;

        for (Json::ArrayIndex p = 0; p < prompts.size(); ++p) {
            std::string prompt = prompts[p].isString() ? prompts[p].asString() : "";
            if (prompt.empty()) continue;
            Json::Value entry;
            entry["model"] = model;
            entry["provider"] = provider;
            entry["prompt_index"] = static_cast<int>(p);

            std::string body = build_chat_body(model, prompt, params, tools, tool_choice);
            auto t0 = std::chrono::steady_clock::now();
            auto res = chat_fn(model, provider, body);
            auto t1 = std::chrono::steady_clock::now();
            auto latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
            entry["latency_ms"] = static_cast<Json::Int64>(latency_ms);

            if (!res || res->status != 200) {
                entry["error"] = res ? std::to_string(res->status) : "request_failed";
                entry["score"] = 0;
                entry["tokens"] = 0;
                entry["tokens_per_second"] = 0.0;
                results.append(entry);
                continue;
            }

            Json::Value resp_json;
            Json::CharReaderBuilder rb;
            std::istringstream iss(res->body);
            if (!Json::parseFromStream(rb, iss, &resp_json, nullptr)) {
                entry["error"] = "parse_error";
                entry["score"] = 0;
                entry["tokens"] = 0;
                entry["tokens_per_second"] = 0.0;
                results.append(entry);
                continue;
            }

            std::string content;
            int tokens = 0;
            if (resp_json.isMember("choices") && resp_json["choices"].size() > 0) {
                const auto& c = resp_json["choices"][0];
                if (c.isMember("message") && c["message"].isMember("content") && c["message"]["content"].isString()) {
                    content = c["message"]["content"].asString();
                }
                if (resp_json.isMember("usage") && resp_json["usage"].isMember("total_tokens")) {
                    tokens = resp_json["usage"]["total_tokens"].asInt();
                }
            } else if (resp_json.isMember("message") && resp_json["message"].isMember("content")) {
                if (resp_json["message"]["content"].isString())
                    content = resp_json["message"]["content"].asString();
                tokens = resp_json.get("eval_count", 0).asInt();
                tokens += resp_json.get("prompt_eval_count", 0).asInt();
            }
            entry["response"] = content;
            entry["tokens"] = tokens;
            if (latency_ms > 0 && tokens > 0) {
                entry["tokens_per_second"] = static_cast<double>(tokens) * 1000.0 / latency_ms;
            } else {
                entry["tokens_per_second"] = 0.0;
            }

            // Evaluation
            std::string eval_prompt = build_eval_prompt(prompt, content);
            std::string eval_body = build_chat_body(eval_model, eval_prompt, Json::objectValue, nullptr, nullptr);
            auto eval_res = chat_fn(eval_model, eval_provider, eval_body);
            EvalResult er;
            if (eval_res && eval_res->status == 200) {
                Json::Value ev_json;
                std::istringstream ev_iss(eval_res->body);
                if (Json::parseFromStream(rb, ev_iss, &ev_json, nullptr)) {
                    std::string ev_content;
                    if (ev_json.isMember("choices") && ev_json["choices"].size() > 0) {
                        const auto& ec = ev_json["choices"][0];
                        if (ec.isMember("message") && ec["message"].isMember("content") && ec["message"]["content"].isString()) {
                            ev_content = ec["message"]["content"].asString();
                        }
                    } else if (ev_json.isMember("message") && ev_json["message"].isMember("content") && ev_json["message"]["content"].isString()) {
                        ev_content = ev_json["message"]["content"].asString();
                    }
                    er = parse_evaluation(ev_content);
                }
            }
            entry["score"] = er.score;
            entry["relevance"] = er.relevance;
            entry["coherence"] = er.coherence;
            entry["conciseness"] = er.conciseness;
            entry["reason"] = er.reason;

            total_latency[model] += latency_ms;
            double tps = (latency_ms > 0 && tokens > 0) ? static_cast<double>(tokens) * 1000.0 / latency_ms : 0.0;
            total_tokens_per_sec[model] += tps;
            total_score[model] += er.score;
            count[model]++;

            results.append(entry);
        }
    }

    out["results"] = results;

    // Summary
    std::string fastest = "";
    double min_lat = 1e9;
    std::string best = "";
    int max_score = -1;
    std::string highest_throughput = "";
    double max_tps = 0.0;
    for (const auto& kv : count) {
        const std::string& mod = kv.first;
        int c = kv.second;
        if (c > 0) {
            double avg_lat = total_latency[mod] / c;
            if (avg_lat < min_lat) { min_lat = avg_lat; fastest = mod; }
            int sc = total_score[mod] / c;
            if (sc > max_score) { max_score = sc; best = mod; }
            double avg_tps = total_tokens_per_sec[mod] / c;
            if (avg_tps > max_tps) { max_tps = avg_tps; highest_throughput = mod; }
        }
    }
    Json::Value summary;
    summary["best_model"] = best;
    summary["fastest_model"] = fastest;
    summary["highest_quality_model"] = best;
    summary["highest_throughput_model"] = highest_throughput;
    out["summary"] = summary;

    return out;
}
