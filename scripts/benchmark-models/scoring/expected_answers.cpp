#include "expected_answers.h"
#include <cctype>
#include <cmath>

namespace benchmark {

static std::string norm(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '\t' || c == '\n' || c == '\r') out += ' ';
        else if (static_cast<unsigned char>(c) <= ' ') continue;
        else out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    while (!out.empty() && out.back() == ' ') out.pop_back();
    while (!out.empty() && out.front() == ' ') out.erase(0, 1);
    return out;
}

ExpectedAnswer getExpectedAnswer(int subject, int complexity, int q_idx) {
    ExpectedAnswer a;
    if (subject == 0) { // general
        if (complexity == 0) {
            const char* expected[] = { "paris", "7", "blue", "shakespeare", "4", "mars", "pacific", "366", "h2o", "da vinci" };
            if (q_idx >= 0 && q_idx < 10) { a.exact = norm(expected[q_idx]); a.has_expected = true; }
        }
    }
    if (subject == 1) { // math
        if (complexity == 0) {
            double basic[] = { 8, 48, 55, 20, 20, 24, 8, 120, 81, 0.25 };
            if (q_idx >= 0 && q_idx < 10) { a.numbers.push_back(basic[q_idx]); a.has_expected = true; }
        } else if (complexity == 1) {
            double medium[] = { 12, 60, 25, 60, 5, 5, 12, 30, 42, 5 };
            if (q_idx >= 0 && q_idx < 10) { a.numbers.push_back(medium[q_idx]); a.has_expected = true; }
        } else {
            double advanced[] = { 0, 0, 0, 0, 120, 210, 3, 5, 0, 3 }; // 0 = expression, skip; 5th=120, 6th=210, 7th=3, 8th=5, 10th=3
            if (q_idx == 4) { a.numbers.push_back(120); a.has_expected = true; }
            if (q_idx == 5) { a.numbers.push_back(210); a.has_expected = true; }
            if (q_idx == 6) { a.numbers.push_back(3); a.has_expected = true; }
            if (q_idx == 7) { a.numbers.push_back(5); a.has_expected = true; }
            if (q_idx == 9) { a.numbers.push_back(3); a.has_expected = true; }
        }
    }
    if (subject == 2 && complexity == 0 && q_idx == 2) { a.exact = "carol"; a.has_expected = true; }
    if (subject == 2 && complexity == 0 && q_idx == 5) { a.exact = "true"; a.has_expected = true; }
    if (subject == 2 && complexity == 0 && q_idx == 8) { a.exact = "no"; a.has_expected = true; }
    return a;
}

} // namespace benchmark
