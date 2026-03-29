#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <atomic>
#include <optional>

struct RateLimiter {
    int max_rpm = 0;

    struct Bucket {
        int tokens = 0;
        int rpm = 0;
        std::chrono::steady_clock::time_point last_refill{};
    };

    struct RateLimitInfo {
        int limit = 0;
        int remaining = 0;
        int64_t reset_ms = 0;
    };

    std::optional<std::unordered_map<std::string, Bucket>> buckets_;
    std::mutex mtx;
    std::atomic<int> call_count{0};

    explicit RateLimiter(int rpm = 0) noexcept : max_rpm(rpm) {}

    void setMaxRpm(int rpm) noexcept { max_rpm = rpm; }

    [[nodiscard]] bool enabled() const noexcept { return max_rpm > 0; }

    [[nodiscard]] RateLimitInfo getInfo(const std::string& id) {
        if (!buckets_) return {max_rpm, max_rpm, 0};
        std::lock_guard lock(mtx);
        auto it = buckets_->find(id);
        if (it == buckets_->end()) return {max_rpm, max_rpm, 0};
        auto& b = it->second;
        int effective = b.rpm > 0 ? b.rpm : max_rpm;
        if (effective <= 0) return {};
        int missing = effective - b.tokens;
        int64_t reset = (missing > 0)
            ? static_cast<int64_t>(missing) * 60000 / effective : 0;
        return {effective, b.tokens, reset};
    }

    [[nodiscard]] bool allow(const std::string& id, int custom_rpm = 0) {
        int effective = (custom_rpm > 0) ? custom_rpm : max_rpm;
        if (effective <= 0) return true;
        if (!buckets_) buckets_.emplace();
        if (++call_count % 1000 == 0) cleanup();

        std::lock_guard lock(mtx);
        auto now = std::chrono::steady_clock::now();
        auto& b = (*buckets_)[id];

        if (b.last_refill == std::chrono::steady_clock::time_point{} || b.rpm != effective) {
            b.tokens = effective;
            b.rpm = effective;
            b.last_refill = now;
        }

        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - b.last_refill).count();
        if (elapsed_ms > 0) {
            auto refill = static_cast<int>(elapsed_ms * effective / 60000);
            b.tokens = std::min(effective, b.tokens + refill);
            b.last_refill = now;
        }

        if (b.tokens > 0) { b.tokens--; return true; }
        return false;
    }

    RateLimiter(const RateLimiter&) = delete;
    RateLimiter& operator=(const RateLimiter&) = delete;

private:
    void cleanup() {
        if (!buckets_) return;
        std::lock_guard lock(mtx);
        auto now = std::chrono::steady_clock::now();
        std::erase_if(*buckets_, [&](const auto& pair) {
            return std::chrono::duration_cast<std::chrono::minutes>(
                now - pair.second.last_refill).count() > 5;
        });
    }
};
