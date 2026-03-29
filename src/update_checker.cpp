#include "update_checker.h"
#include "httplib.h"
#include "logger.h"
#include <sstream>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <cstdlib>
#include <charconv>
#include <algorithm>
#include <filesystem>

namespace {

// Comparação semver simples: retorna -1 se a < b, 0 se a == b, 1 se a > b.
int compare_versions(const std::string& a, const std::string& b) {
    auto parse = [](const std::string& s) -> std::vector<int> {
        std::vector<int> parts;
        size_t start = 0;
        for (;;) {
            size_t end = s.find('.', start);
            std::string seg = (end == std::string::npos)
                ? s.substr(start) : s.substr(start, end - start);
            int v = 0;
            auto [ptr, ec] = std::from_chars(
                seg.data(), seg.data() + seg.size(), v);
            if (ec != std::errc{} || seg.empty()) parts.push_back(0);
            else parts.push_back(v);
            if (end == std::string::npos) break;
            start = end + 1;
        }
        return parts;
    };
    std::vector<int> pa = parse(a);
    std::vector<int> pb = parse(b);
    size_t n = (std::max)(pa.size(), pb.size());
    for (size_t i = 0; i < n; ++i) {
        int va = (i < pa.size()) ? pa[i] : 0;
        int vb = (i < pb.size()) ? pb[i] : 0;
        if (va < vb) return -1;
        if (va > vb) return 1;
    }
    return 0;
}

// Extrai base URL (scheme + host + port) e path de uma URL completa.
bool parse_url(const std::string& url,
               std::string& base_scheme_host_port,
               std::string& path) {
    if (url.empty()) return false;
    size_t scheme_end = url.find("://");
    if (scheme_end == std::string::npos) return false;
    size_t authority_start = scheme_end + 3;
    size_t path_start = url.find('/', authority_start);
    if (path_start == std::string::npos) {
        base_scheme_host_port = url;
        path = "/";
        return true;
    }
    base_scheme_host_port = url.substr(0, path_start);
    path = url.substr(path_start);
    if (path.empty()) path = "/";
    return true;
}

} // namespace

Json::Value UpdateChecker::check(const std::string& check_url,
                                 const Json::Value& current_plugins) {
    Json::Value out(Json::objectValue);
    out["core"] = Json::objectValue;
    out["core"]["current"] = UpdateChecker::core_version();
    out["core"]["available"] = false;
    out["plugins"] = Json::arrayValue;

    if (check_url.empty()) {
        out["error"] = "update_check_disabled";
        out["message"] = "Configure updates.check_url or UPDATE_CHECK_URL to enable update checks.";
        return out;
    }

    std::string base, path;
    if (!parse_url(check_url, base, path)) {
        out["error"] = "invalid_check_url";
        out["message"] = "Malformed update check URL.";
        return out;
    }

    httplib::Client cli(base);
    cli.set_connection_timeout(5, 0);
    cli.set_read_timeout(10, 0);
    auto res = cli.Get(path);
    if (!res) {
        out["error"] = "network_error";
        out["message"] = "Failed to fetch update manifest.";
        Json::Value f;
        f["url"] = check_url;
        Logger::warn("update_check_fetch_failed", f);
        return out;
    }
    if (res->status != 200) {
        out["error"] = "manifest_error";
        out["message"] = "Update manifest returned status " + std::to_string(res->status);
        return out;
    }

    Json::Value manifest;
    {
        Json::CharReaderBuilder builder;
        std::string errs;
        std::istringstream stream(res->body);
        if (!Json::parseFromStream(builder, stream, &manifest, &errs)) {
            out["error"] = "manifest_parse_error";
            out["message"] = "Invalid JSON manifest: " + errs;
            return out;
        }
    }

    std::string current_core = UpdateChecker::core_version();

    // Core
    if (manifest.isMember("core") && manifest["core"].isObject()) {
        const auto& core = manifest["core"];
        if (core.isMember("version")) {
            std::string latest = core["version"].asString();
            out["core"]["latest"] = latest;
            if (compare_versions(current_core, latest) < 0) {
                out["core"]["available"] = true;
                if (core.isMember("release_notes")) out["core"]["release_notes"] = core["release_notes"];
                if (core.isMember("download_url")) out["core"]["download_url"] = core["download_url"];
            }
        }
    }

    // Plugins: manifest pode ter "plugins" array com { name, version, download_url? }
    std::unordered_map<std::string, std::string> plugin_latest;
    if (manifest.isMember("plugins") && manifest["plugins"].isArray()) {
        for (const auto& entry : manifest["plugins"]) {
            if (entry.isObject() && entry.isMember("name") && entry.isMember("version"))
                plugin_latest[entry["name"].asString()] = entry["version"].asString();
        }
    }

    if (current_plugins.isArray()) {
        for (const auto& p : current_plugins) {
            if (!p.isObject() || !p.isMember("name") || !p.isMember("version")) continue;
            std::string name = p["name"].asString();
            std::string current_ver = p["version"].asString();
            Json::Value plug(Json::objectValue);
            plug["name"] = name;
            plug["current"] = current_ver;
            plug["available"] = false;
            auto it = plugin_latest.find(name);
            if (it != plugin_latest.end()) {
                plug["latest"] = it->second;
                if (compare_versions(current_ver, it->second) < 0) {
                    plug["available"] = true;
                    for (const auto& entry : manifest["plugins"]) {
                        if (entry.isObject() && entry["name"].asString() == name
                            && entry.isMember("download_url"))
                            plug["download_url"] = entry["download_url"];
                    }
                }
            }
            out["plugins"].append(plug);
        }
    }

    return out;
}

std::string UpdateChecker::apply_download(const std::string& download_url,
                                          const std::string& staged_path) {
    if (download_url.empty()) return "download_url is empty";
    if (staged_path.empty()) return "staged_path is not configured";

    std::string base, path;
    if (!parse_url(download_url, base, path))
        return "invalid download URL";

    httplib::Client cli(base);
    cli.set_connection_timeout(10, 0);
    cli.set_read_timeout(120, 0);  // binário pode ser grande
    auto res = cli.Get(path);
    if (!res)
        return "network error fetching binary";
    if (res->status != 200)
        return "download returned status " + std::to_string(res->status);

    std::string tmp_path = staged_path + ".tmp";
    {
        std::ofstream out(tmp_path, std::ios::binary);
        if (!out)
            return "cannot write to " + tmp_path;
        out.write(res->body.data(), static_cast<std::streamsize>(res->body.size()));
        if (!out)
            return "write failed to " + tmp_path;
    }

    std::error_code ec;
    if (std::filesystem::exists(staged_path, ec) && !ec)
        std::filesystem::remove(staged_path, ec);
    if (ec)
        return "cannot remove existing staged file: " + ec.message();
    std::filesystem::rename(tmp_path, staged_path, ec);
    if (ec)
        return "cannot rename to staged path: " + ec.message();

    return "";
}
