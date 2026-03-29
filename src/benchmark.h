#pragma once
#include <string>
#include <functional>
#include <json/json.h>
#include "httplib.h"

class BenchmarkRunner {
public:
    using ChatFn = std::function<httplib::Result(
        const std::string& model,
        const std::string& provider,
        const std::string& body)>;

    struct EvalResult {
        int score = 0;
        int relevance = 0;
        int coherence = 0;
        int conciseness = 0;
        std::string reason;
    };

    /** Run benchmark synchronously. Returns full result JSON. */
    static Json::Value run(
        const Json::Value& request,
        ChatFn chat_fn);

    /** Parse evaluator LLM response into EvalResult. */
    static EvalResult parse_evaluation(const std::string& llm_response);
};
