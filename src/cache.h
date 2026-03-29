#pragma once
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <chrono>
#include <optional>
#include <list>
#include <atomic>
#include <cstdint>
#include <functional>

struct ResponseCache {
    struct Stats {
        uint64_t hits;
        uint64_t misses;
        uint64_t evictions;
        size_t entries;
        size_t memory_bytes;
        double hit_rate;
    };

    bool cache_enabled = false;
    int ttl_seconds = 300;
    size_t max_entries = 1000;
    size_t max_memory_bytes = 0;

    [[nodiscard]] bool enabled() const noexcept { return cache_enabled; }

    // ── Hash helpers (avoid temp string allocation) ────────────────────

    static size_t combineHash(size_t seed, size_t value) noexcept {
        return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    }
    static size_t combineHash(size_t seed, const std::string& s) noexcept {
        return combineHash(seed, std::hash<std::string>{}(s));
    }
    static size_t combineHash(size_t seed, double d) noexcept {
        return combineHash(seed, std::hash<double>{}(d));
    }
    static size_t combineHash(size_t seed, int i) noexcept {
        return combineHash(seed, std::hash<int>{}(i));
    }

    // ── Key generation ─────────────────────────────────────────────────

    struct CacheKey {
        size_t hash;
        std::string full;
    };

    [[nodiscard]] static CacheKey makeChatKey(
            const std::string& model,
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

    [[nodiscard]] static CacheKey makeEmbedKey(
            const std::string& model,
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

    // ── Lookup ─────────────────────────────────────────────────────────

    [[nodiscard]] std::optional<std::string> get(const CacheKey& key) {
        if (!enabled() || !index_) return std::nullopt;
        std::unique_lock lock(mtx_);
        auto it = index_->find(key.hash);
        if (it == index_->end()) {
            misses_++;
            return std::nullopt;
        }

        auto& entry = *it->second;
        if (entry.full_key != key.full) {
            misses_++;
            return std::nullopt;
        }

        auto age = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - entry.created).count();
        if (age > ttl_seconds) {
            memory_bytes_ -= entry.mem_size;
            lru_->erase(it->second);
            index_->erase(it);
            misses_++;
            return std::nullopt;
        }

        lru_->splice(lru_->begin(), *lru_, it->second);
        hits_++;
        return entry.response;
    }

    // ── Insert ─────────────────────────────────────────────────────────

    void put(const CacheKey& key, const std::string& response) {
        if (!enabled()) return;
        std::unique_lock lock(mtx_);
        if (!lru_) { lru_.emplace(); index_.emplace(); }

        size_t entry_mem = response.size() + key.full.size();

        auto existing = index_->find(key.hash);
        if (existing != index_->end()) {
            if (existing->second->full_key == key.full) {
                memory_bytes_ -= existing->second->mem_size;
                existing->second->response = response;
                existing->second->mem_size = entry_mem;
                existing->second->created = std::chrono::steady_clock::now();
                memory_bytes_ += entry_mem;
                lru_->splice(lru_->begin(), *lru_, existing->second);
                return;
            }
            memory_bytes_ -= existing->second->mem_size;
            lru_->erase(existing->second);
            index_->erase(existing);
            evictions_++;
        }

        if (max_memory_bytes > 0) {
            while (memory_bytes_ + entry_mem > max_memory_bytes && !lru_->empty())
                evictLRU();
        }
        while (index_->size() >= max_entries && !lru_->empty())
            evictLRU();

        lru_->push_front({key.hash, key.full, response, entry_mem,
                         std::chrono::steady_clock::now()});
        (*index_)[key.hash] = lru_->begin();
        memory_bytes_ += entry_mem;

        if (++put_count_ % CLEANUP_INTERVAL == 0)
            cleanupExpired();
    }

    // ── Stats ──────────────────────────────────────────────────────────

    [[nodiscard]] Stats stats() const {
        std::shared_lock lock(mtx_);
        auto h = hits_.load();
        auto m = misses_.load();
        auto total = h + m;
        size_t entries = index_ ? index_->size() : 0;
        return {h, m, evictions_.load(),
                entries, memory_bytes_,
                total > 0 ? static_cast<double>(h) / static_cast<double>(total) : 0.0};
    }

private:
    static constexpr uint64_t CLEANUP_INTERVAL = 100;

    struct Entry {
        size_t hash_key;
        std::string full_key;
        std::string response;
        size_t mem_size;
        std::chrono::steady_clock::time_point created;
    };

    std::optional<std::list<Entry>> lru_;
    std::optional<std::unordered_map<size_t, std::list<Entry>::iterator>> index_;
    size_t memory_bytes_ = 0;
    uint64_t put_count_ = 0;
    mutable std::shared_mutex mtx_;

    std::atomic<uint64_t> hits_{0};
    std::atomic<uint64_t> misses_{0};
    std::atomic<uint64_t> evictions_{0};

    void evictLRU() {
        auto& victim = lru_->back();
        memory_bytes_ -= victim.mem_size;
        index_->erase(victim.hash_key);
        lru_->pop_back();
        evictions_++;
    }

    void cleanupExpired() {
        if (!lru_) return;
        auto now = std::chrono::steady_clock::now();
        auto it = lru_->begin();
        while (it != lru_->end()) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                now - it->created).count();
            if (age > ttl_seconds) {
                memory_bytes_ -= it->mem_size;
                index_->erase(it->hash_key);
                it = lru_->erase(it);
                evictions_++;
            } else {
                ++it;
            }
        }
    }
};
