#pragma once
#include "provider.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

class ProviderRouter {
public:
    void register_provider(std::unique_ptr<Provider> provider);
    void set_model_mapping(const std::string& model, const std::string& provider_name);
    void set_fallback_chain(std::vector<std::string> provider_names);

    /** Resolve provider for model. Returns nullptr if no provider. */
    [[nodiscard]] Provider* resolve(const std::string& model) const;

    /** Get provider by name (e.g. "ollama", "openrouter"). */
    [[nodiscard]] Provider* get_provider(const std::string& provider_name) const;

    /** Next provider in fallback chain after given provider, or nullptr. */
    [[nodiscard]] Provider* fallback_after(const std::string& provider_name) const;

    /** First provider in fallback chain (default when model not mapped). */
    [[nodiscard]] Provider* default_provider() const;

    [[nodiscard]] bool has_providers() const { return !providers_.empty(); }

private:
    std::vector<std::unique_ptr<Provider>> providers_;
    std::unordered_map<std::string, Provider*> by_name_;
    std::unordered_map<std::string, std::string> model_to_provider_;
    std::vector<std::string> fallback_chain_;
};
