#ifndef BENCHMARK_MATH_VALIDATOR_H
#define BENCHMARK_MATH_VALIDATOR_H

#include "expected_answers.h"
#include <string>

namespace benchmark {

// Extract numeric value(s) from model output and check against expected.
bool mathCorrect(const std::string& modelOutput, const ExpectedAnswer& expected, double tolerance = 0.01);

// Extract first number found in text (integer or decimal).
double extractNumber(const std::string& text);

} // namespace benchmark

#endif
