#include "logic_validator.h"
#include <cctype>
#include <algorithm>

namespace benchmark {

static std::string lower(const std::string& s) {
    std::string out;
    for (char c : s) out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

LogicScores logicScores(const std::string& modelOutput, const std::string& prompt) {
    LogicScores s;
    std::string out = lower(modelOutput);
    std::string p = lower(prompt);

    if (out.find("therefore") != std::string::npos || out.find("thus") != std::string::npos ||
        out.find("hence") != std::string::npos || out.find("so ") != std::string::npos)
        s.structural_coherence = std::min(1.0, s.structural_coherence + 0.4);
    if (out.find("first") != std::string::npos || out.find("second") != std::string::npos ||
        out.find("step") != std::string::npos || out.find("1.") != std::string::npos ||
        out.find("2.") != std::string::npos)
        s.step_by_step = std::min(1.0, s.step_by_step + 0.5);
    if (out.size() > 20) s.structural_coherence = std::min(1.0, s.structural_coherence + 0.2);

    s.no_obvious_contradiction = 1.0; // no simple check without NLI
    if (out.size() >= 10 && out.size() <= 2000) s.adheres_to_constraint = 0.7;
    if (out.size() > 2000) s.adheres_to_constraint = 0.4; // possibly verbose
    return s;
}

} // namespace benchmark
