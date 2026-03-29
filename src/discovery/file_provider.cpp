#include "file_provider.h"
#include "../logger.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <yaml-cpp/yaml.h>

namespace discovery {

FileProvider::FileProvider(FileProviderConfig config)
    : config_(std::move(config)) {
    status_.name = "file";
}

FileProvider::~FileProvider() { stop(); }

bool FileProvider::get_file_stat(const std::string& path, struct stat& st) {
    return ::stat(path.c_str(), &st) == 0;
}

bool FileProvider::ends_with(const std::string& s, const std::string& suffix) {
    if (suffix.size() > s.size()) return false;
    return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void FileProvider::scan_directory(const std::string& dir,
                                  std::vector<std::string>& out) const {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;
            auto ext = entry.path().extension().string();
            if (ext == ".yaml" || ext == ".yml" || ext == ".json")
                out.push_back(entry.path().string());
        }
        std::sort(out.begin(), out.end());
    } catch (const std::exception& e) {
        Json::Value f;
        f["dir"] = dir;
        f["error"] = e.what();
        Logger::warn("discovery_file_scan_error", f);
    }
}

DynamicConfigFragment FileProvider::parse_yaml_file(const std::string& path) const {
    DynamicConfigFragment frag;
    frag.provider = "file";

    try {
        YAML::Node root = YAML::LoadFile(path);

        if (root["backends"] && root["backends"].IsMap()) {
            for (auto it = root["backends"].begin(); it != root["backends"].end(); ++it) {
                DiscoveredBackend b;
                b.name = it->first.as<std::string>();
                b.qualified_id = b.name + "@file";

                auto node = it->second;
                b.type = node["type"] ? node["type"].as<std::string>() : "ollama";

                if (node["url"])
                    b.base_url = node["url"].as<std::string>();
                else if (node["base_url"])
                    b.base_url = node["base_url"].as<std::string>();

                if (node["api_key_env"])
                    b.api_key_env = node["api_key_env"].as<std::string>();
                if (node["default_model"])
                    b.default_model = node["default_model"].as<std::string>();
                if (node["weight"])
                    b.weight = node["weight"].as<int>();

                if (node["models"] && node["models"].IsSequence()) {
                    for (const auto& m : node["models"])
                        b.models.push_back(m.as<std::string>());
                }

                frag.backends.push_back(std::move(b));
            }
        }

        if (root["model_routing"] && root["model_routing"].IsMap()) {
            for (auto it = root["model_routing"].begin();
                 it != root["model_routing"].end(); ++it) {
                frag.model_routing[it->first.as<std::string>()] =
                    it->second.as<std::string>();
            }
        }

        if (root["model_aliases"] && root["model_aliases"].IsMap()) {
            for (auto it = root["model_aliases"].begin();
                 it != root["model_aliases"].end(); ++it) {
                frag.model_aliases[it->first.as<std::string>()] =
                    it->second.as<std::string>();
            }
        }

        if (root["fallback_chain"] && root["fallback_chain"].IsSequence()) {
            for (const auto& f : root["fallback_chain"])
                frag.fallback_chain.push_back(f.as<std::string>());
        }

    } catch (const std::exception& e) {
        Json::Value f;
        f["path"] = path;
        f["error"] = e.what();
        Logger::error("discovery_file_parse_yaml_error", f);
    }

    return frag;
}

DynamicConfigFragment FileProvider::parse_json_file(const std::string& path) const {
    DynamicConfigFragment frag;
    frag.provider = "file";

    try {
        std::ifstream file(path);
        if (!file.is_open()) return frag;

        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errs;
        if (!Json::parseFromStream(builder, file, &root, &errs)) {
            Json::Value f;
            f["path"] = path;
            f["error"] = errs;
            Logger::error("discovery_file_parse_json_error", f);
            return frag;
        }

        if (root.isMember("backends") && root["backends"].isObject()) {
            for (const auto& name : root["backends"].getMemberNames()) {
                DiscoveredBackend b;
                b.name = name;
                b.qualified_id = name + "@file";
                auto& node = root["backends"][name];
                b.type = node.get("type", "ollama").asString();
                if (node.isMember("url"))
                    b.base_url = node["url"].asString();
                else if (node.isMember("base_url"))
                    b.base_url = node["base_url"].asString();
                b.api_key_env = node.get("api_key_env", "").asString();
                b.default_model = node.get("default_model", "").asString();
                b.weight = node.get("weight", 1).asInt();
                if (node.isMember("models") && node["models"].isArray()) {
                    for (const auto& m : node["models"])
                        b.models.push_back(m.asString());
                }
                frag.backends.push_back(std::move(b));
            }
        }

        if (root.isMember("model_routing") && root["model_routing"].isObject()) {
            for (const auto& key : root["model_routing"].getMemberNames())
                frag.model_routing[key] = root["model_routing"][key].asString();
        }

        if (root.isMember("model_aliases") && root["model_aliases"].isObject()) {
            for (const auto& key : root["model_aliases"].getMemberNames())
                frag.model_aliases[key] = root["model_aliases"][key].asString();
        }

        if (root.isMember("fallback_chain") && root["fallback_chain"].isArray()) {
            for (const auto& f : root["fallback_chain"])
                frag.fallback_chain.push_back(f.asString());
        }

    } catch (const std::exception& e) {
        Json::Value f;
        f["path"] = path;
        f["error"] = e.what();
        Logger::error("discovery_file_parse_json_error", f);
    }

    return frag;
}

DynamicConfigFragment FileProvider::parse_file(const std::string& path) const {
    if (ends_with(path, ".json"))
        return parse_json_file(path);
    return parse_yaml_file(path);
}

DynamicConfigFragment FileProvider::provide() {
    auto start = std::chrono::steady_clock::now();

    DynamicConfigFragment merged;
    merged.provider = "file";

    std::vector<std::string> files;
    for (const auto& p : config_.paths) {
        struct stat st {};
        if (get_file_stat(p, st)) {
            if (S_ISDIR(st.st_mode))
                scan_directory(p, files);
            else
                files.push_back(p);
        }
    }

    for (const auto& f : files) {
        auto frag = parse_file(f);
        for (auto& b : frag.backends)
            merged.backends.push_back(std::move(b));
        for (auto& [k, v] : frag.model_routing)
            merged.model_routing[k] = std::move(v);
        for (auto& [k, v] : frag.model_aliases)
            merged.model_aliases[k] = std::move(v);
        for (auto& fc : frag.fallback_chain)
            merged.fallback_chain.push_back(std::move(fc));
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    {
        std::lock_guard lk(status_mu_);
        status_.healthy = true;
        status_.last_error.clear();
        status_.backends_discovered = static_cast<int>(merged.backends.size());
        status_.last_refresh = std::chrono::steady_clock::now();
        status_.last_poll_latency = elapsed;
    }

    return merged;
}

bool FileProvider::detect_changes() {
    std::vector<std::string> files;
    for (const auto& p : config_.paths) {
        struct stat st {};
        if (get_file_stat(p, st)) {
            if (S_ISDIR(st.st_mode))
                scan_directory(p, files);
            else
                files.push_back(p);
        }
    }

    bool changed = false;

    if (files.size() != tracked_files_.size()) {
        changed = true;
    } else {
        for (size_t i = 0; i < files.size(); ++i) {
            struct stat st {};
            if (!get_file_stat(files[i], st)) continue;
            if (i >= tracked_files_.size()
                || tracked_files_[i].path != files[i]
                || tracked_files_[i].mtime != st.st_mtime
                || tracked_files_[i].size != st.st_size) {
                changed = true;
                break;
            }
        }
    }

    if (changed) {
        tracked_files_.clear();
        tracked_files_.reserve(files.size());
        for (const auto& f : files) {
            struct stat st {};
            if (get_file_stat(f, st))
                tracked_files_.push_back({f, st.st_mtime, st.st_size});
        }
    }

    return changed;
}

void FileProvider::poll_loop() {
    detect_changes();

    while (running_.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(
            std::chrono::seconds(config_.poll_interval_seconds));
        if (!running_.load(std::memory_order_relaxed)) break;

        if (detect_changes() && on_change_) {
            Json::Value f;
            f["files_tracked"] = static_cast<int>(tracked_files_.size());
            Logger::info("discovery_file_change_detected", f);
            on_change_();
        }
    }
}

void FileProvider::watch(std::function<void()> on_change) {
    on_change_ = std::move(on_change);
    running_.store(true, std::memory_order_relaxed);
    watch_thread_ = std::thread(&FileProvider::poll_loop, this);
}

void FileProvider::stop() {
    running_.store(false, std::memory_order_relaxed);
    if (watch_thread_.joinable())
        watch_thread_.join();
}

ProviderStatus FileProvider::status() const {
    std::lock_guard lk(status_mu_);
    return status_;
}

} // namespace discovery
