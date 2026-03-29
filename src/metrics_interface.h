#pragma once
#include <string>
#include <json/json.h>
#include "core_services.h"

class IMetricsCollector {
public:
    virtual ~IMetricsCollector() = default;

    virtual void begin_request() = 0;
    virtual void end_request() = 0;

    [[nodiscard]] virtual std::string toJson(const core::ICache* cache,
        const Json::Value& plugin_metrics = Json::Value()) const = 0;
    [[nodiscard]] virtual std::string toPrometheus(const core::ICache* cache,
        const Json::Value& plugin_metrics = Json::Value()) const = 0;

    struct ActiveGuard {
        IMetricsCollector* m;
        explicit ActiveGuard(IMetricsCollector* mc) noexcept : m(mc) {
            if (m) m->begin_request();
        }
        ~ActiveGuard() noexcept {
            if (m) m->end_request();
        }
        ActiveGuard(const ActiveGuard&) = delete;
        ActiveGuard& operator=(const ActiveGuard&) = delete;
        ActiveGuard(ActiveGuard&&) = delete;
        ActiveGuard& operator=(ActiveGuard&&) = delete;
    };
};
