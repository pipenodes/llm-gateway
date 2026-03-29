#ifndef BENCHMARK_LOGIC_VALIDATOR_H
#define BENCHMARK_LOGIC_VALIDATOR_H

#include <string>

namespace benchmark {

// Heuristic scores 0..1 for cognitive/quality aspects (no ground truth).
struct LogicScores {
    double structural_coherence = 0.0;  // has structure (e.g. "therefore", "thus")
    double step_by_step = 0.0;         // numbered or sequential reasoning
    double no_obvious_contradiction = 0.0;
    double adheres_to_constraint = 0.0; // stays on topic
};

LogicScores logicScores(const std::string& modelOutput, const std::string& prompt);

} // namespace benchmark

#endif
