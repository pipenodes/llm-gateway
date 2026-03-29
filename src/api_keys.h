#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <fstream>
#include <optional>
#include <atomic>
#include <charconv>
#include <json/json.h>
#include "logger.h"
#include "crypto.h"

// ── IP Whitelist with CIDR ─────────────────────────────────────────────

struct IpWhitelist {
    [[nodiscard]] static std::string normalizeIp(std::string_view ip) noexcept {
        if (ip == "::1") return "127.0.0.1";
        if (ip.starts_with("::ffff:")) return std::string(ip.substr(7));
        return std::string(ip);
    }

    [[nodiscard]] static uint32_t parseIpv4(std::string_view ip) noexcept {
        uint32_t result = 0;
        uint32_t octet = 0;
        int shift = 24;
        int dots = 0;
        bool has_digit = false;
        for (char c : ip) {
            if (c == '.') {
                if (!has_digit || octet > 255) return 0;
                result |= (octet << shift);
                shift -= 8;
                octet = 0;
                has_digit = false;
                dots++;
                if (dots > 3) return 0;
            } else if (c >= '0' && c <= '9') {
                octet = octet * 10 + static_cast<uint32_t>(c - '0');
                has_digit = true;
            } else {
                return 0;
            }
        }
        if (!has_digit || octet > 255 || dots != 3) return 0;
        result |= (octet << shift);
        return result;
    }

    [[nodiscard]] static bool isIpv6(std::string_view ip) noexcept {
        return ip.find(':') != std::string_view::npos;
    }

    [[nodiscard]] static bool isAllowed(std::string_view raw_ip,
                                        const std::vector<std::string>& whitelist) noexcept {
        if (whitelist.empty()) return true;
        auto ip = normalizeIp(raw_ip);
        if (isIpv6(ip)) return false;
        uint32_t client = parseIpv4(ip);
        if (client == 0 && ip != "0.0.0.0") return false;
        for (const auto& entry : whitelist) {
            auto slash = entry.find('/');
            if (slash == std::string::npos) {
                if (parseIpv4(entry) == client) return true;
            } else {
                uint32_t base = parseIpv4(entry.substr(0, slash));
                int prefix = 0;
                auto pstr = std::string_view(entry).substr(slash + 1);
                auto [ptr, ec] = std::from_chars(pstr.data(), pstr.data() + pstr.size(), prefix);
                if (ec != std::errc{} || prefix < 0 || prefix > 32) continue;
                uint32_t mask = (prefix == 0) ? 0 : (~uint32_t(0)) << (32 - prefix);
                if ((client & mask) == (base & mask)) return true;
            }
        }
        return false;
    }
};

// ── API Key Info ───────────────────────────────────────────────────────

struct ApiKeyInfo {
    std::string key_hash;
    std::string key_prefix;
    std::string alias;
    int64_t created_at = 0;
    int64_t last_used_at = 0;
    uint64_t request_count = 0;
    int rate_limit_rpm = 0;
    std::vector<std::string> ip_whitelist;
    bool active = true;
};

// ── API Key Manager ────────────────────────────────────────────────────

class ApiKeyManager {
    static constexpr int KEY_LENGTH = 48;
    static constexpr std::string_view CHARSET =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    std::optional<std::vector<ApiKeyInfo>> keys_;
    std::optional<std::unordered_map<std::string, size_t>> by_hash_;
    std::optional<std::unordered_map<std::string, size_t>> by_alias_;
    mutable std::shared_mutex mtx_;
    std::string path_;
    mutable std::atomic<bool> loaded_{false};

public:
    ApiKeyManager() = default;
    ApiKeyManager(const ApiKeyManager&) = delete;
    ApiKeyManager& operator=(const ApiKeyManager&) = delete;

    void init(const std::string& path = "keys.json");

    [[nodiscard]] bool hasActiveKeys() const {
        ensureLoaded();
        std::shared_lock lock(mtx_);
        if (!keys_) return false;
        for (const auto& k : *keys_)
            if (k.active) return true;
        return false;
    }

    [[nodiscard]] size_t activeKeyCount() const {
        ensureLoaded();
        std::shared_lock lock(mtx_);
        if (!keys_) return 0;
        size_t n = 0;
        for (const auto& k : *keys_)
            if (k.active) n++;
        return n;
    }

    struct AuthResult {
        std::string alias;
        int rate_limit_rpm = 0;
        std::vector<std::string> ip_whitelist;
    };

    [[nodiscard]] std::optional<AuthResult> validate(const std::string& bearer_token);

    struct CreateResult {
        bool ok = false;
        std::string error;
        std::string key;
        std::string alias;
        int64_t created_at = 0;
    };

    [[nodiscard]] CreateResult create(const std::string& alias,
                                      int rate_limit_rpm = 0,
                                      const std::vector<std::string>& ip_whitelist = {});

    [[nodiscard]] bool revoke(const std::string& alias);
    [[nodiscard]] Json::Value list() const;

private:
    [[nodiscard]] static std::string genKey();
    void ensureLoaded() const;
    void rebuildIndices();
    void load();
    void saveUnlocked();
};
