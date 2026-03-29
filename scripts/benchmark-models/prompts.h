/**
 * Benchmark prompts: 6 subjects × 3 complexity levels × 10 questions each.
 * Inspired by MMLU, GSM8K, HumanEval, MBPP, and common AI evaluation benchmarks.
 */
#ifndef BENCHMARK_MODELS_PROMPTS_H
#define BENCHMARK_MODELS_PROMPTS_H

#include <string>
#include <vector>

namespace benchmark {

enum class Subject { General, Math, LogicalReasoning, Python, JavaScript, CSharp };
enum class Complexity { Basic, Medium, Advanced };

inline const char* subjectName(Subject s) {
    switch (s) {
        case Subject::General: return "general";
        case Subject::Math: return "math";
        case Subject::LogicalReasoning: return "logical_reasoning";
        case Subject::Python: return "python";
        case Subject::JavaScript: return "javascript";
        case Subject::CSharp: return "csharp";
    }
    return "?";
}

inline const char* complexityName(Complexity c) {
    switch (c) {
        case Complexity::Basic: return "basic";
        case Complexity::Medium: return "medium";
        case Complexity::Advanced: return "advanced";
    }
    return "?";
}

// Returns prompts[subject][complexity] = vector of 10 questions
std::vector<std::vector<std::vector<std::string>>> getAllPrompts();

} // namespace benchmark

#endif
