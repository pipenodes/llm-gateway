#include "api_keys.h"

void ApiKeyManager::init(const std::string& path) {
    path_ = path;
}

void ApiKeyManager::ensureLoaded() const {
    if (loaded_.load(std::memory_order_acquire)) return;
    std::unique_lock lock(mtx_);
    if (loaded_.load(std::memory_order_relaxed)) return;
    const_cast<ApiKeyManager*>(this)->load();
    loaded_.store(true, std::memory_order_release);
}

std::optional<ApiKeyManager::AuthResult> ApiKeyManager::validate(const std::string& bearer_token) {
    ensureLoaded();
    auto token_hash = crypto::sha256_hex(bearer_token);
    std::unique_lock lock(mtx_);
    if (!by_hash_) return std::nullopt;
    auto it = by_hash_->find(token_hash);
    if (it == by_hash_->end()) return std::nullopt;
    auto& k = (*keys_)[it->second];
    if (!k.active) return std::nullopt;
    k.last_used_at = crypto::epoch_seconds();
    k.request_count++;
    if (k.request_count % 100 == 0) saveUnlocked();
    return AuthResult{k.alias, k.rate_limit_rpm, k.ip_whitelist};
}

ApiKeyManager::CreateResult ApiKeyManager::create(const std::string& alias,
                                                  int rate_limit_rpm,
                                                  const std::vector<std::string>& ip_whitelist) {
    if (alias.empty())
        return CreateResult{.ok = false, .error = "alias is required"};

    ensureLoaded();
    std::unique_lock lock(mtx_);
    if (!keys_) { keys_.emplace(); by_hash_.emplace(); by_alias_.emplace(); }
    if (by_alias_->count(alias))
        return CreateResult{.ok = false, .error = "alias already exists"};

    auto raw_key = genKey();

    ApiKeyInfo info;
    info.key_hash = crypto::sha256_hex(raw_key);
    info.key_prefix = raw_key.substr(0, 8);
    info.alias = alias;
    info.created_at = crypto::epoch_seconds();
    info.rate_limit_rpm = rate_limit_rpm;
    info.ip_whitelist = ip_whitelist;

    size_t idx = keys_->size();
    keys_->push_back(std::move(info));
    (*by_hash_)[(*keys_)[idx].key_hash] = idx;
    (*by_alias_)[(*keys_)[idx].alias] = idx;
    saveUnlocked();

    Json::Value f; f["alias"] = (*keys_)[idx].alias;
    Logger::info("key_created", f);
    return CreateResult{.ok = true, .error = "", .key = raw_key, .alias = (*keys_)[idx].alias, .created_at = (*keys_)[idx].created_at};
}

bool ApiKeyManager::revoke(const std::string& alias) {
    ensureLoaded();
    std::unique_lock lock(mtx_);
    if (!by_alias_) return false;
    auto it = by_alias_->find(alias);
    if (it == by_alias_->end()) return false;
    if (!(*keys_)[it->second].active) return false;
    (*keys_)[it->second].active = false;
    saveUnlocked();

    Json::Value f; f["alias"] = alias;
    Logger::info("key_revoked", f);
    return true;
}

Json::Value ApiKeyManager::list() const {
    ensureLoaded();
    std::shared_lock lock(mtx_);
    Json::Value out(Json::arrayValue);
    if (!keys_) return out;
    for (const auto& k : *keys_) {
        Json::Value e;
        e["alias"] = k.alias;
        e["key_prefix"] = k.key_prefix + "****";
        e["created_at"] = static_cast<Json::Int64>(k.created_at);
        e["last_used_at"] = static_cast<Json::Int64>(k.last_used_at);
        e["request_count"] = static_cast<Json::UInt64>(k.request_count);
        e["rate_limit_rpm"] = k.rate_limit_rpm;
        e["active"] = k.active;
        Json::Value wl(Json::arrayValue);
        for (const auto& ip : k.ip_whitelist) wl.append(ip);
        e["ip_whitelist"] = wl;
        out.append(e);
    }
    return out;
}

std::string ApiKeyManager::genKey() {
    return "sk-" + crypto::secure_random(KEY_LENGTH, CHARSET);
}

void ApiKeyManager::rebuildIndices() {
    if (!keys_) return;
    if (!by_hash_) { by_hash_.emplace(); by_alias_.emplace(); }
    by_hash_->clear();
    by_alias_->clear();
    for (size_t i = 0; i < keys_->size(); ++i) {
        (*by_hash_)[(*keys_)[i].key_hash] = i;
        (*by_alias_)[(*keys_)[i].alias] = i;
    }
}

void ApiKeyManager::load() {
    std::ifstream file(path_);
    if (!file.is_open()) return;
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    if (!Json::parseFromStream(builder, file, &root, &errs) || !root.isArray()) return;

    if (!keys_) { keys_.emplace(); by_hash_.emplace(); by_alias_.emplace(); }
    keys_->clear();
    bool needs_migration = false;
    for (const auto& item : root) {
        ApiKeyInfo k;
        k.alias = item.get("alias", "").asString();
        k.created_at = item.get("created_at", 0).asInt64();
        k.last_used_at = item.get("last_used_at", 0).asInt64();
        k.request_count = item.get("request_count", 0).asUInt64();
        k.rate_limit_rpm = item.get("rate_limit_rpm", 0).asInt();
        k.active = item.get("active", true).asBool();
        if (item.isMember("ip_whitelist") && item["ip_whitelist"].isArray())
            for (const auto& ip : item["ip_whitelist"])
                k.ip_whitelist.push_back(ip.asString());

        if (item.isMember("key_hash")) {
            k.key_hash = item["key_hash"].asString();
            k.key_prefix = item.get("key_prefix", "").asString();
        } else if (item.isMember("key")) {
            auto raw_key = item["key"].asString();
            k.key_hash = crypto::sha256_hex(raw_key);
            k.key_prefix = raw_key.substr(0, std::min(raw_key.size(), size_t(8)));
            needs_migration = true;
        } else {
            continue;
        }

        if (!k.key_hash.empty() && !k.alias.empty())
            keys_->push_back(std::move(k));
    }
    rebuildIndices();

    if (needs_migration) {
        saveUnlocked();
        Logger::info("keys_migrated_to_hash");
    }

    Json::Value f; f["count"] = static_cast<int>(keys_->size());
    Logger::info("keys_loaded", f);
}

void ApiKeyManager::saveUnlocked() {
    if (!keys_) return;
    Json::StreamWriterBuilder w;
    w["indentation"] = "  ";
    Json::Value root(Json::arrayValue);
    for (const auto& k : *keys_) {
        Json::Value item;
        item["key_hash"] = k.key_hash;
        item["key_prefix"] = k.key_prefix;
        item["alias"] = k.alias;
        item["created_at"] = static_cast<Json::Int64>(k.created_at);
        item["last_used_at"] = static_cast<Json::Int64>(k.last_used_at);
        item["request_count"] = static_cast<Json::UInt64>(k.request_count);
        item["rate_limit_rpm"] = k.rate_limit_rpm;
        item["active"] = k.active;
        Json::Value wl(Json::arrayValue);
        for (const auto& ip : k.ip_whitelist) wl.append(ip);
        item["ip_whitelist"] = wl;
        root.append(item);
    }
    std::ofstream file(path_);
    if (file.is_open())
        file << Json::writeString(w, root) << '\n';
}
