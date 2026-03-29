#pragma once
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <future>
#include <chrono>
#include <json/json.h>
#include <cstddef>
#include <cstdint>

// ── Cache key (compatible with ResponseCache::CacheKey) ───────────────────

namespace core {

struct CacheKey {
    size_t hash;
    std::string full;
};

inline size_t combineHash(size_t seed, size_t value) noexcept {
    return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}
inline size_t combineHash(size_t seed, const std::string& s) noexcept {
    return combineHash(seed, std::hash<std::string>{}(s));
}
inline size_t combineHash(size_t seed, double d) noexcept {
    return combineHash(seed, std::hash<double>{}(d));
}
inline size_t combineHash(size_t seed, int i) noexcept {
    return combineHash(seed, std::hash<int>{}(i));
}

inline CacheKey makeChatKey(const std::string& model,
                            const std::string& messages_json,
                            double temperature, double top_p, int max_tokens) {
    size_t h = combineHash(0, model);
    h = combineHash(h, messages_json);
    h = combineHash(h, temperature);
    h = combineHash(h, top_p);
    h = combineHash(h, max_tokens);
    std::string full;
    full.reserve(model.size() + messages_json.size() + 48);
    full += "chat|";
    full += model;
    full += '|';
    full += messages_json;
    full += "|t=";
    full += std::to_string(temperature);
    full += "|p=";
    full += std::to_string(top_p);
    full += "|m=";
    full += std::to_string(max_tokens);
    return {h, std::move(full)};
}

inline CacheKey makeEmbedKey(const std::string& model,
                             const std::string& input_json) {
    size_t h = combineHash(0, model);
    h = combineHash(h, input_json);
    h = combineHash(h, std::string("embed"));
    std::string full;
    full.reserve(model.size() + input_json.size() + 8);
    full += "embed|";
    full += model;
    full += '|';
    full += input_json;
    return {h, std::move(full)};
}

// ── ICache ───────────────────────────────────────────────────────────────

struct CacheStats {
    uint64_t hits = 0;
    uint64_t misses = 0;
    uint64_t evictions = 0;
    size_t entries = 0;
    size_t memory_bytes = 0;
    double hit_rate = 0.0;
};

class ICache {
public:
    virtual ~ICache() = default;
    [[nodiscard]] virtual bool enabled() const noexcept = 0;
    [[nodiscard]] virtual std::optional<std::string> get(const CacheKey& key) = 0;
    virtual void put(const CacheKey& key, const std::string& response) = 0;
    [[nodiscard]] virtual CacheStats cacheStats() const = 0;
};

// ── IAuth ───────────────────────────────────────────────────────────────

struct AuthResult {
    std::string alias;
    int rate_limit_rpm = 0;
    std::vector<std::string> ip_whitelist;
};

struct AuthCreateResult {
    bool ok = false;
    std::string error;
    std::string key;
    std::string alias;
    int64_t created_at = 0;
};

class IAuth {
public:
    virtual ~IAuth() = default;
    [[nodiscard]] virtual bool hasActiveKeys() const = 0;
    [[nodiscard]] virtual size_t activeKeyCount() const = 0;
    [[nodiscard]] virtual std::optional<AuthResult> validate(const std::string& bearer_token) = 0;
    [[nodiscard]] virtual AuthCreateResult create(const std::string& alias,
        int rate_limit_rpm = 0,
        const std::vector<std::string>& ip_whitelist = {}) = 0;
    [[nodiscard]] virtual bool revoke(const std::string& alias) = 0;
    [[nodiscard]] virtual Json::Value list() const = 0;
};

// ── IRateLimiter ─────────────────────────────────────────────────────────

struct RateLimitInfo {
    int limit = 0;
    int remaining = 0;
    int64_t reset_ms = 0;
};

class IRateLimiter {
public:
    virtual ~IRateLimiter() = default;
    [[nodiscard]] virtual bool enabled() const noexcept = 0;
    virtual void setMaxRpm(int rpm) noexcept = 0;
    [[nodiscard]] virtual RateLimitInfo getInfo(const std::string& id) = 0;
    [[nodiscard]] virtual bool allow(const std::string& id, int custom_rpm = 0) = 0;
};

// ── IRequestQueue ────────────────────────────────────────────────────────

enum class Priority : int {
    Critical = 0,
    High = 1,
    Normal = 2,
    Low = 3
};

struct EnqueueResult {
    bool accepted = false;
    size_t position = 0;
    std::future<bool> completion;
};

class IRequestQueue {
public:
    virtual ~IRequestQueue() = default;
    [[nodiscard]] virtual bool enabled() const noexcept = 0;
    [[nodiscard]] virtual EnqueueResult enqueue(Priority priority,
        std::chrono::milliseconds timeout,
        std::function<void()> task,
        const std::string& request_id = "",
        const std::string& key_alias = "") = 0;
    [[nodiscard]] virtual size_t pending() const noexcept = 0;
    [[nodiscard]] virtual Json::Value stats() const = 0;
    virtual void clear_pending() = 0;
    virtual void shutdown() = 0;
};

// ── IAuditSink (request completed event) ─────────────────────────────────
// Implementations must be thread-safe and should not block (prefer queue + worker).
// Registered plugins implementing IAuditSink receive on_request_completed after each request.

struct AuditEntry {
    int64_t timestamp = 0;
    std::string request_id;
    std::string key_alias;
    std::string client_ip;
    std::string method;
    std::string path;
    std::string model;
    int status_code = 0;
    int prompt_tokens = 0;
    int completion_tokens = 0;
    int64_t latency_ms = 0;
    bool stream = false;
    bool cache_hit = false;
    std::string error;
};

class IAuditSink {
public:
    virtual ~IAuditSink() = default;
    /** Called after each request (from request thread). Must be thread-safe and non-blocking. */
    virtual void on_request_completed(const AuditEntry& entry) = 0;
};

struct AuditQuery {
    std::string key_alias;
    std::string model;
    int64_t from_timestamp = 0;
    int64_t to_timestamp = 0;
    int status_code = 0;
    int limit = 100;
    int offset = 0;
};

/** Optional: plugin that can serve GET /admin/audit. */
class IAuditQueryProvider {
public:
    virtual ~IAuditQueryProvider() = default;
    [[nodiscard]] virtual Json::Value query(const AuditQuery& q) const = 0;
};

// ── Null implementations ────────────────────────────────────────────────

class NullCache : public ICache {
public:
    [[nodiscard]] bool enabled() const noexcept override { return false; }
    [[nodiscard]] std::optional<std::string> get(const CacheKey&) override { return std::nullopt; }
    void put(const CacheKey&, const std::string&) override {}
    [[nodiscard]] CacheStats cacheStats() const override { return {}; }
};

class NullAuth : public IAuth {
public:
    [[nodiscard]] bool hasActiveKeys() const override { return false; }
    [[nodiscard]] size_t activeKeyCount() const override { return 0; }
    [[nodiscard]] std::optional<AuthResult> validate(const std::string&) override { return std::nullopt; }
    [[nodiscard]] AuthCreateResult create(const std::string&, int, const std::vector<std::string>&) override {
        AuthCreateResult r;
        r.ok = false;
        r.error = "Auth not configured";
        return r;
    }
    [[nodiscard]] bool revoke(const std::string&) override { return false; }
    [[nodiscard]] Json::Value list() const override { return Json::Value(Json::arrayValue); }
};

class NullRateLimiter : public IRateLimiter {
public:
    [[nodiscard]] bool enabled() const noexcept override { return false; }
    void setMaxRpm(int) noexcept override {}
    [[nodiscard]] RateLimitInfo getInfo(const std::string&) override { return {}; }
    [[nodiscard]] bool allow(const std::string&, int) override { return true; }
};

class NullRequestQueue : public IRequestQueue {
public:
    [[nodiscard]] bool enabled() const noexcept override { return false; }
    [[nodiscard]] EnqueueResult enqueue(Priority, std::chrono::milliseconds,
        std::function<void()>, const std::string&, const std::string&) override;
    [[nodiscard]] size_t pending() const noexcept override { return 0; }
    [[nodiscard]] Json::Value stats() const override { return Json::Value(Json::objectValue); }
    void clear_pending() override {}
    void shutdown() override {}
};

} // namespace core
