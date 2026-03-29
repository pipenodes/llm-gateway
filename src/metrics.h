#pragma once
#include <atomic>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <json/json.h>
#include "core_services.h"
#include "metrics_interface.h"

struct MetricsCollector : IMetricsCollector {
    std::atomic<uint64_t> requests_total{0};
    std::atomic<int32_t> requests_active{0};
    std::chrono::steady_clock::time_point start_time;

    MetricsCollector() noexcept : start_time(std::chrono::steady_clock::now()) {}

    void begin_request() override {
        requests_total++;
        requests_active++;
    }
    void end_request() override {
        requests_active--;
    }

    struct MemInfo { long rss_kb = -1; long peak_kb = -1; };

    [[nodiscard]] MemInfo readProcMem() const noexcept {
        MemInfo info;
#ifdef __linux__
        std::ifstream file("/proc/self/status");
        if (!file.is_open()) return info;
        std::string line;
        while (std::getline(file, line)) {
            if (line.starts_with("VmRSS:")) {
                std::istringstream iss(line);
                std::string label;
                iss >> label >> info.rss_kb;
            } else if (line.starts_with("VmPeak:")) {
                std::istringstream iss(line);
                std::string label;
                iss >> label >> info.peak_kb;
            }
            if (info.rss_kb >= 0 && info.peak_kb >= 0) break;
        }
#endif
        return info;
    }

    [[nodiscard]] int64_t uptimeSeconds() const noexcept {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time).count();
    }

    [[nodiscard]] std::string toJson(const core::ICache* cache,
        const Json::Value& plugin_metrics = Json::Value()) const override {
        auto mem = readProcMem();
        thread_local Json::StreamWriterBuilder w = [] {
            Json::StreamWriterBuilder b;
            b["indentation"] = "  ";
            return b;
        }();
        Json::Value out;
        out["uptime_seconds"] = static_cast<Json::Int64>(uptimeSeconds());
        out["memory_rss_kb"] = static_cast<Json::Int64>(mem.rss_kb);
        out["memory_peak_kb"] = static_cast<Json::Int64>(mem.peak_kb);
        out["requests_total"] = static_cast<Json::UInt64>(requests_total.load());
        out["requests_active"] = requests_active.load();

        if (cache && cache->enabled()) {
            auto cs = cache->cacheStats();
            Json::Value c;
            c["enabled"] = true;
            c["hits"] = static_cast<Json::UInt64>(cs.hits);
            c["misses"] = static_cast<Json::UInt64>(cs.misses);
            c["evictions"] = static_cast<Json::UInt64>(cs.evictions);
            c["entries"] = static_cast<Json::UInt64>(cs.entries);
            c["memory_bytes"] = static_cast<Json::UInt64>(cs.memory_bytes);
            c["hit_rate"] = cs.hit_rate;
            out["cache"] = c;
        } else {
            out["cache"]["enabled"] = false;
        }

        out["plugins"] = (plugin_metrics.isObject() && !plugin_metrics.empty())
            ? plugin_metrics : Json::objectValue;

        return Json::writeString(w, out);
    }

    [[nodiscard]] std::string toPrometheus(const core::ICache* cache,
        const Json::Value& plugin_metrics = Json::Value()) const override {
        auto mem = readProcMem();
        std::ostringstream ss;
        ss << "# HELP hermes_requests_total Total HTTP requests\n"
           << "# TYPE hermes_requests_total counter\n"
           << "hermes_requests_total " << requests_total.load() << "\n\n"
           << "# HELP hermes_requests_active Currently processing\n"
           << "# TYPE hermes_requests_active gauge\n"
           << "hermes_requests_active " << requests_active.load() << "\n\n"
           << "# HELP hermes_memory_rss_bytes RSS in bytes\n"
           << "# TYPE hermes_memory_rss_bytes gauge\n"
           << "hermes_memory_rss_bytes " << (mem.rss_kb > 0 ? mem.rss_kb * 1024 : -1) << "\n\n"
           << "# HELP hermes_memory_peak_bytes Peak memory in bytes\n"
           << "# TYPE hermes_memory_peak_bytes gauge\n"
           << "hermes_memory_peak_bytes " << (mem.peak_kb > 0 ? mem.peak_kb * 1024 : -1) << "\n\n"
           << "# HELP hermes_uptime_seconds Uptime in seconds\n"
           << "# TYPE hermes_uptime_seconds gauge\n"
           << "hermes_uptime_seconds " << uptimeSeconds() << "\n\n";

        if (cache && cache->enabled()) {
            auto cs = cache->cacheStats();
            ss << "# HELP hermes_cache_hits Total cache hits\n"
               << "# TYPE hermes_cache_hits counter\n"
               << "hermes_cache_hits " << cs.hits << "\n\n"
               << "# HELP hermes_cache_misses Total cache misses\n"
               << "# TYPE hermes_cache_misses counter\n"
               << "hermes_cache_misses " << cs.misses << "\n\n"
               << "# HELP hermes_cache_evictions Total cache evictions\n"
               << "# TYPE hermes_cache_evictions counter\n"
               << "hermes_cache_evictions " << cs.evictions << "\n\n"
               << "# HELP hermes_cache_entries Current cache entries\n"
               << "# TYPE hermes_cache_entries gauge\n"
               << "hermes_cache_entries " << cs.entries << "\n\n"
               << "# HELP hermes_cache_memory_bytes Cache memory usage\n"
               << "# TYPE hermes_cache_memory_bytes gauge\n"
               << "hermes_cache_memory_bytes " << cs.memory_bytes << "\n\n"
               << "# HELP hermes_cache_hit_rate Cache hit rate\n"
               << "# TYPE hermes_cache_hit_rate gauge\n"
               << "hermes_cache_hit_rate " << cs.hit_rate << "\n";
        }

        if (plugin_metrics.isObject() && plugin_metrics.isMember("semantic_cache")) {
            const auto& sc = plugin_metrics["semantic_cache"];
            if (sc.isMember("hits") && sc.isMember("misses")) {
                auto hits = sc["hits"].asInt64();
                auto misses = sc["misses"].asInt64();
                auto entries = sc.get("entries", Json::Value(0)).asInt64();
                ss << "# HELP hermes_semantic_cache_hits Semantic cache hits\n"
                   << "# TYPE hermes_semantic_cache_hits counter\n"
                   << "hermes_semantic_cache_hits " << hits << "\n\n"
                   << "# HELP hermes_semantic_cache_misses Semantic cache misses\n"
                   << "# TYPE hermes_semantic_cache_misses counter\n"
                   << "hermes_semantic_cache_misses " << misses << "\n\n"
                   << "# HELP hermes_semantic_cache_entries Semantic cache entries\n"
                   << "# TYPE hermes_semantic_cache_entries gauge\n"
                   << "hermes_semantic_cache_entries " << entries << "\n";
            }
        }

        return ss.str();
    }
};
