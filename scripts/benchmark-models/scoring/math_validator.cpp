#include "math_validator.h"
#include <cctype>
#include <cmath>
#include <sstream>

namespace benchmark {

double extractNumber(const std::string& text) {
    std::string numStr;
    bool seenDot = false;
    bool started = false;
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (std::isdigit(static_cast<unsigned char>(c))) {
            numStr += c;
            started = true;
        } else if (c == '.' && !seenDot && started) {
            numStr += c;
            seenDot = true;
        } else if ((c == '-' || c == '+') && numStr.empty() && !started) {
            numStr += c;
            started = true;
        } else if (started && !std::isdigit(static_cast<unsigned char>(c)) && c != '.' && c != ' ')
            break;
    }
    if (numStr.empty()) return 0.0;
    double v = 0.0;
    std::istringstream is(numStr);
    if (is >> v) return v;
    return 0.0;
}

bool mathCorrect(const std::string& modelOutput, const ExpectedAnswer& expected, double tolerance) {
    if (!expected.has_expected || expected.numbers.empty()) return false;
    double got = extractNumber(modelOutput);
    for (double exp : expected.numbers) {
        if (std::fabs(got - exp) <= tolerance) return true;
        if (std::fabs(exp) >= 1 && std::fabs((got - exp) / exp) <= tolerance) return true;
    }
    return false;
}

} // namespace benchmark
