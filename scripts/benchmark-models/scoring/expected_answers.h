#ifndef BENCHMARK_EXPECTED_ANSWERS_H
#define BENCHMARK_EXPECTED_ANSWERS_H

#include <string>
#include <vector>

namespace benchmark {

// Expected answer for scoring. For math: numeric value(s); for others: exact string or empty.
struct ExpectedAnswer {
    std::string exact;           // exact match string (normalized)
    std::vector<double> numbers; // for math: accepted numeric answers (e.g. 42 or 42.0)
    bool has_expected = false;
};

// subject 0=general, 1=math, 2=logic, 3=python, 4=javascript, 5=csharp
// complexity 0=basic, 1=medium, 2=advanced
// q_idx 0..9
ExpectedAnswer getExpectedAnswer(int subject, int complexity, int q_idx);

} // namespace benchmark

#endif
