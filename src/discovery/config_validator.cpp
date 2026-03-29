#include "config_validator.h"
#include <unordered_set>

namespace discovery {

ValidationResult ConfigValidator::validate(const MergedDynamicConfig& cfg) {
    ValidationResult result;

    std::unordered_set<std::string> backend_ids;
    std::unordered_set<std::string> backend_names;

    for (const auto& b : cfg.backends) {
        if (b.qualified_id.empty()) {
            result.valid = false;
            result.errors.push_back("Backend with empty qualified_id");
            continue;
        }
        if (!backend_ids.insert(b.qualified_id).second) {
            result.valid = false;
            result.errors.push_back("Duplicate backend qualified_id: " + b.qualified_id);
        }
        backend_names.insert(b.name);

        if (b.base_url.empty()) {
            result.valid = false;
            result.errors.push_back("Backend '" + b.qualified_id + "' has empty base_url");
        }
        if (b.type != "ollama" && b.type != "openai") {
            result.valid = false;
            result.errors.push_back("Backend '" + b.qualified_id + "' has invalid type: " + b.type);
        }
        if (b.models.empty()) {
            result.warnings.push_back("Backend '" + b.qualified_id + "' has no models declared");
        }
    }

    for (const auto& [model, target] : cfg.model_routing) {
        bool found = false;
        for (const auto& b : cfg.backends) {
            if (b.qualified_id == target || b.name == target) {
                found = true;
                break;
            }
        }
        if (!found) {
            result.valid = false;
            result.errors.push_back("model_routing: model '" + model
                + "' routes to unknown backend '" + target + "'");
        }
    }

    for (const auto& fc_entry : cfg.fallback_chain) {
        bool found = false;
        for (const auto& b : cfg.backends) {
            if (b.qualified_id == fc_entry || b.name == fc_entry) {
                found = true;
                break;
            }
        }
        if (!found) {
            result.valid = false;
            result.errors.push_back("fallback_chain references unknown backend: " + fc_entry);
        }
    }

    // Detect alias cycles: follow each alias chain up to N hops
    for (const auto& [alias, _target] : cfg.model_aliases) {
        std::unordered_set<std::string> visited;
        std::string current = alias;
        bool cycle = false;
        for (int i = 0; i < 100; ++i) {
            if (!visited.insert(current).second) {
                cycle = true;
                break;
            }
            auto it = cfg.model_aliases.find(current);
            if (it == cfg.model_aliases.end()) break;
            current = it->second;
        }
        if (cycle) {
            result.valid = false;
            result.errors.push_back("model_aliases: cycle detected starting from '" + alias + "'");
        }
    }

    return result;
}

} // namespace discovery
