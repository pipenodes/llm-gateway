#ifndef BENCHMARK_SCORE_UTILS_H
#define BENCHMARK_SCORE_UTILS_H

#include "expected_answers.h"
#include <string>

namespace benchmark {

std::string normalizeForMatch(const std::string& s);
bool exactMatch(const std::string& modelOutput, const ExpectedAnswer& expected);

// Tokenize by spaces and punctuation skip.
void tokenize(const std::string& s, std::vector<std::string>& out);
// F1: overlap tokens vs reference (reference = expected.exact tokenized).
double f1Score(const std::string& modelOutput, const std::string& reference);
// Simplified BLEU-like: geometric mean of 1-gram..4-gram precisions (no brevity penalty).
double simpleBleu(const std::string& modelOutput, const std::string& reference);

} // namespace benchmark

#endif
