#pragma once

// Enterprise loader for HERMES (llm-gateway).
//
// Components:
//  hermes::LicenseValidator   — validates PIPELEAP_LICENSE_KEY against pipenodes, 72h grace
//  hermes::PluginInstaller    — downloads + verifies SHA256 + hot-loads .so
//  hermes::MarketplaceWatcher — SSE client for plugin update notifications

#include "edge_config.h"

#if ENTERPRISE_ENABLED

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <functional>
#include <stdexcept>

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <openssl/sha.h>

namespace hermes {

namespace detail {

inline std::string getenv_str(const char* key, const char* def = "") {
    const char* v = std::getenv(key);
    return v ? v : def;
}

inline std::pair<std::string, int> parse_url(const std::string& url) {
    std::string h = url;
    if (h.rfind("http://", 0) == 0)  h = h.substr(7);
    if (h.rfind("https://", 0) == 0) h = h.substr(8);
    auto colon = h.find_last_of(':');
    if (colon == std::string::npos) return {h, 80};
    int port = 80;
    try { port = std::stoi(h.substr(colon + 1)); } catch (...) {}
    return {h.substr(0, colon), port};
}

inline std::string sha256_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open " + path);
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    char buf[8192];
    while (f.read(buf, sizeof(buf)) || f.gcount())
        SHA256_Update(&ctx, buf, static_cast<size_t>(f.gcount()));
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_Final(digest, &ctx);
    char hex[SHA256_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        std::snprintf(hex + i * 2, 3, "%02x", digest[i]);
    return hex;
}

} // namespace detail

// ── LicenseValidator ──────────────────────────────────────────────────────────

class LicenseValidator {
public:
    static constexpr int VALIDATION_INTERVAL_H = 1;
    static constexpr int GRACE_PERIOD_H        = 72;

    struct LicenseState {
        bool        valid = false;
        std::string plan_id;
        std::string org_id;
        std::vector<std::string> features;
    };

    using StateCallback = std::function<void(const LicenseState&)>;

    void start(StateCallback on_change = nullptr) {
        key_       = detail::getenv_str("PIPELEAP_LICENSE_KEY");
        nodes_url_ = detail::getenv_str("PIPELEAP_PIPENODES_URL", "http://localhost:8103");
        managed_   = detail::getenv_str("PIPELEAP_MANAGED") == "1" ||
                     detail::getenv_str("PIPELEAP_MANAGED") == "true";
        on_change_ = std::move(on_change);

        if (managed_) {
            spdlog::info("HERMES: managed deployment — license check delegated to control plane");
            state_.valid = true;
            return;
        }

        if (key_.empty()) {
            spdlog::warn("HERMES: PIPELEAP_LICENSE_KEY not set — enterprise plugins disabled");
            spdlog::warn("  Get a license key at https://pipenodes.io");
            return;
        }

        validate_once();

        running_ = true;
        thread_ = std::thread([this] {
            while (running_) {
                for (int m = 0; m < VALIDATION_INTERVAL_H * 60 && running_; ++m)
                    std::this_thread::sleep_for(std::chrono::minutes(1));
                if (running_) validate_once();
            }
        });

        spdlog::info("HERMES: license validator started (interval={}h, grace={}h)",
                     VALIDATION_INTERVAL_H, GRACE_PERIOD_H);
    }

    void stop() {
        running_ = false;
        if (thread_.joinable()) thread_.join();
    }

    ~LicenseValidator() { stop(); }

    bool is_valid()             const { return managed_ || state_.valid; }
    const LicenseState& state() const { return state_; }

private:
    std::string   key_, nodes_url_;
    bool          managed_ = false;
    std::atomic<bool> running_{false};
    std::thread   thread_;
    LicenseState  state_;
    StateCallback on_change_;
    std::chrono::steady_clock::time_point last_success_{};
    bool          had_success_ = false;

    void validate_once() {
        try {
            auto [host, port] = detail::parse_url(nodes_url_);
            httplib::Client cli(host, port);
            cli.set_connection_timeout(10);
            cli.set_read_timeout(10);

            nlohmann::json body = { {"license_key", key_} };
            auto res = cli.Post("/licenses/validate", body.dump(), "application/json");

            if (res && res->status == 200) {
                auto j   = nlohmann::json::parse(res->body);
                bool was = state_.valid;
                state_.valid    = j.value("valid", false);
                state_.plan_id  = j.value("plan_id", "");
                state_.org_id   = j.value("org_id",  "");
                state_.features = j.value("features", std::vector<std::string>{});

                if (state_.valid) {
                    had_success_  = true;
                    last_success_ = std::chrono::steady_clock::now();
                    spdlog::info("HERMES: license valid (plan={} org={})",
                                 state_.plan_id, state_.org_id);
                } else {
                    spdlog::warn("HERMES: license rejected by pipenodes");
                }

                if (was != state_.valid && on_change_) on_change_(state_);
                return;
            }
            spdlog::warn("HERMES: pipenodes unreachable ({}:{})", host, port);
        } catch (const std::exception& e) {
            spdlog::warn("HERMES: license validation error: {}", e.what());
        }

        if (had_success_) {
            auto elapsed_h = static_cast<int>(
                std::chrono::duration_cast<std::chrono::hours>(
                    std::chrono::steady_clock::now() - last_success_).count());
            if (elapsed_h < GRACE_PERIOD_H) {
                spdlog::warn("HERMES: pipenodes unreachable — grace period active ({}h / {}h)",
                             elapsed_h, GRACE_PERIOD_H);
            } else {
                spdlog::error("HERMES: grace period expired — enterprise plugins disabled");
                bool was = state_.valid;
                state_.valid = false;
                if (was && on_change_) on_change_(state_);
            }
        }
    }
};

// ── PluginInstaller ───────────────────────────────────────────────────────────

class PluginInstaller {
public:
    // pm_unload / pm_load are lambdas to avoid coupling to a specific PluginManager type.
    // For llm-gateway, pass wrappers around gateway.plugin_manager.
    PluginInstaller(std::function<void(const std::string&)> unload_fn,
                    std::function<void(const std::string&)> load_fn,
                    std::string enterprise_dir)
        : unload_(std::move(unload_fn)), load_(std::move(load_fn)),
          dir_(std::move(enterprise_dir)) {}

    bool install(const std::string& plugin_name,
                 const std::string& download_url,
                 const std::string& checksum_sha256 = "") {
        namespace fs = std::filesystem;

        if (!fs::is_directory(dir_)) {
            try { fs::create_directories(dir_); } catch (const std::exception& e) {
                spdlog::error("HERMES: cannot create plugin dir {}: {}", dir_, e.what());
                return false;
            }
        }

        std::string tmp  = (fs::path(dir_) / (plugin_name + ".so.tmp")).string();
        std::string dest = (fs::path(dir_) / (plugin_name + ".so")).string();

        try {
            if (!download(download_url, tmp)) return false;

            if (!checksum_sha256.empty()) {
                std::string actual = detail::sha256_file(tmp);
                if (actual != checksum_sha256) {
                    spdlog::error("HERMES: checksum mismatch for {} (expected={} got={})",
                                  plugin_name, checksum_sha256, actual);
                    fs::remove(tmp);
                    return false;
                }
            }

            fs::rename(tmp, dest);
            unload_(plugin_name);
            load_(dest);
            spdlog::info("HERMES: plugin installed: {}", plugin_name);
            return true;
        } catch (const std::exception& e) {
            spdlog::error("HERMES: plugin install failed for {}: {}", plugin_name, e.what());
            try { fs::remove(tmp); } catch (...) {}
            return false;
        }
    }

private:
    std::function<void(const std::string&)> unload_, load_;
    std::string dir_;

    static bool download(const std::string& url, const std::string& dest) {
        auto [host, port] = detail::parse_url(url);
        std::string path = "/";
        auto slash = url.find('/', url.find("://") + 3);
        if (slash != std::string::npos) path = url.substr(slash);

        httplib::Client cli(host, port);
        cli.set_connection_timeout(30);
        cli.set_read_timeout(120);

        std::ofstream out(dest, std::ios::binary);
        if (!out) return false;

        auto res = cli.Get(path, [&out](const char* data, size_t len) {
            out.write(data, static_cast<std::streamsize>(len));
            return true;
        });
        return res && res->status == 200;
    }
};

// ── MarketplaceWatcher ────────────────────────────────────────────────────────

class MarketplaceWatcher {
public:
    using ReloadCallback = std::function<void(const std::string& plugin_name,
                                               const std::string& download_url,
                                               const std::string& checksum)>;

    MarketplaceWatcher(std::string marketplace_url, ReloadCallback cb)
        : url_(std::move(marketplace_url)), cb_(std::move(cb)) {}

    void start(const std::string& product = "llm-gateway",
               const std::string& core_version = "2.0.0") {
        running_ = true;
        thread_  = std::thread([this, product, core_version] {
            watch(product, core_version);
        });
        spdlog::info("HERMES: MarketplaceWatcher started (url={})", url_);
    }

    void stop() {
        running_ = false;
        if (thread_.joinable()) thread_.join();
    }

    ~MarketplaceWatcher() { stop(); }

private:
    std::string       url_;
    ReloadCallback    cb_;
    std::atomic<bool> running_{false};
    std::thread       thread_;

    void watch(const std::string& product, const std::string& core_version) {
        auto [host, port] = detail::parse_url(url_);
        std::string path  = "/marketplace/updates/stream?product=" + product +
                            "&core_version=" + core_version;
        int backoff_s = 2;

        while (running_) {
            try {
                httplib::Client cli(host, port);
                cli.set_connection_timeout(10);
                cli.set_read_timeout(0);
                cli.Get(path, [this](const char* data, size_t len) {
                    if (!running_) return false;
                    parse_sse(std::string(data, len));
                    return true;
                });
            } catch (const std::exception& e) {
                spdlog::warn("HERMES: MarketplaceWatcher: {}", e.what());
            }
            spdlog::warn("HERMES: MarketplaceWatcher reconnecting in {}s", backoff_s);
            for (int i = 0; i < backoff_s && running_; ++i)
                std::this_thread::sleep_for(std::chrono::seconds(1));
            backoff_s = std::min(backoff_s * 2, 60);
        }
    }

    void parse_sse(const std::string& chunk) {
        size_t pos = chunk.find("data: ");
        if (pos == std::string::npos) return;
        std::string s = chunk.substr(pos + 6);
        auto nl = s.find('\n');
        if (nl != std::string::npos) s = s.substr(0, nl);
        if (s.empty() || s == "{}") return;
        try {
            auto j  = nlohmann::json::parse(s);
            auto nm = j.value("plugin_name", "");
            auto dl = j.value("download_url", "");
            auto cs = j.value("checksum_sha256", "");
            if (!nm.empty() && cb_) {
                spdlog::info("HERMES: MarketplaceWatcher: update for {}", nm);
                cb_(nm, dl, cs);
            }
        } catch (...) {}
    }
};

// Compatibility helpers matching the old interface
inline std::string enterprise_plugin_dir() {
    return detail::getenv_str("PIPELEAP_ENTERPRISE_PLUGIN_DIR", ENTERPRISE_PLUGIN_DIR);
}
inline void log_enterprise_status() {
    spdlog::info("HERMES: enterprise loader active (dir={})", enterprise_plugin_dir());
}

} // namespace hermes

#else // !ENTERPRISE_ENABLED

namespace hermes {
struct LicenseValidator {
    struct LicenseState { bool valid = false; };
    void start(auto&&...) {}
    void stop() {}
    bool is_valid() const { return false; }
};
struct PluginInstaller {
    PluginInstaller(auto&&...) {}
    bool install(auto&&...) { return false; }
};
struct MarketplaceWatcher {
    MarketplaceWatcher(auto&&...) {}
    void start(auto&&...) {}
    void stop() {}
};
inline std::string enterprise_plugin_dir() { return ""; }
inline void log_enterprise_status() {}
} // namespace hermes

#endif // ENTERPRISE_ENABLED
