#include "provider_router.h"

void ProviderRouter::register_provider(std::unique_ptr<Provider> provider) {
    if (!provider) return;
    auto name = provider->name();
    auto* ptr = provider.get();
    by_name_[name] = ptr;
    providers_.push_back(std::move(provider));
}

void ProviderRouter::set_model_mapping(const std::string& model,
                                       const std::string& provider_name) {
    model_to_provider_[model] = provider_name;
}

void ProviderRouter::set_fallback_chain(std::vector<std::string> provider_names) {
    fallback_chain_ = std::move(provider_names);
}

Provider* ProviderRouter::get_provider(const std::string& provider_name) const {
    auto it = by_name_.find(provider_name);
    return (it != by_name_.end()) ? it->second : nullptr;
}

Provider* ProviderRouter::resolve(const std::string& model) const {
    auto it = model_to_provider_.find(model);
    if (it != model_to_provider_.end()) {
        auto pit = by_name_.find(it->second);
        if (pit != by_name_.end())
            return pit->second;
    }

    return default_provider();
}

Provider* ProviderRouter::fallback_after(const std::string& provider_name) const {
    bool found = false;
    for (const auto& name : fallback_chain_) {
        if (found) {
            auto it = by_name_.find(name);
            if (it != by_name_.end())
                return it->second;
        }
        if (name == provider_name)
            found = true;
    }
    return nullptr;
}

Provider* ProviderRouter::default_provider() const {
    for (const auto& name : fallback_chain_) {
        auto it = by_name_.find(name);
        if (it != by_name_.end())
            return it->second;
    }
    if (!providers_.empty())
        return providers_.front().get();
    return nullptr;
}
