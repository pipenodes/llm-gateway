#include "metrics_minimal.h"
#include <json/json.h>
#include <sstream>

void MinimalMetricsCollector::begin_request() {
    requests_total++;
    requests_active++;
}

void MinimalMetricsCollector::end_request() {
    requests_active--;
}

std::string MinimalMetricsCollector::toJson(const core::ICache*,
    const Json::Value& plugin_metrics) const {
    Json::Value out;
    out["uptime_seconds"] = static_cast<Json::Int64>(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - start_time).count());
    out["requests_total"] = static_cast<Json::UInt64>(requests_total.load());
    out["requests_active"] = requests_active.load();
    out["minimal"] = true;
    out["plugins"] = plugin_metrics.isObject() ? plugin_metrics : Json::objectValue;
    Json::StreamWriterBuilder w;
    w["indentation"] = "  ";
    return Json::writeString(w, out);
}

std::string MinimalMetricsCollector::toPrometheus(const core::ICache*,
    const Json::Value&) const {
    std::ostringstream ss;
    int64_t uptime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - start_time).count();
    ss << "# HELP hermes_requests_total Total HTTP requests\n"
       << "# TYPE hermes_requests_total counter\n"
       << "hermes_requests_total " << requests_total.load() << "\n\n"
       << "# HELP hermes_requests_active Currently processing\n"
       << "# TYPE hermes_requests_active gauge\n"
       << "hermes_requests_active " << requests_active.load() << "\n\n"
       << "# HELP hermes_uptime_seconds Uptime in seconds\n"
       << "# TYPE hermes_uptime_seconds gauge\n"
       << "hermes_uptime_seconds " << uptime << "\n";
    return ss.str();
}
