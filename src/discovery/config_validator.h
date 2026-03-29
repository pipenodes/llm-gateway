#pragma once
#include "dynamic_config.h"
#include <string>
#include <vector>

namespace discovery {

struct ValidationResult {
    bool valid = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    [[nodiscard]] Json::Value to_json() const {
        Json::Value j;
        j["valid"] = valid;
        Json::Value errs(Json::arrayValue);
        for (const auto& e : errors) errs.append(e);
        j["errors"] = errs;
        Json::Value warns(Json::arrayValue);
        for (const auto& w : warnings) warns.append(w);
        j["warnings"] = warns;
        return j;
    }
};

class ConfigValidator {
public:
    static ValidationResult validate(const MergedDynamicConfig& cfg);
};

} // namespace discovery
