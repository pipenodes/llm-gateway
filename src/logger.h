#pragma once
#include <iostream>
#include <string>
#include <string_view>
#include <chrono>
#include <json/json.h>

struct Logger {
    [[nodiscard]] static int64_t epoch_seconds() noexcept {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    static void log(std::string_view level, std::string_view event,
                    const Json::Value& fields = Json::Value()) noexcept {
        thread_local Json::StreamWriterBuilder w = [] {
            Json::StreamWriterBuilder b;
            b["indentation"] = "";
            return b;
        }();
        Json::Value entry;
        entry["ts"] = static_cast<Json::Int64>(epoch_seconds());
        entry["level"] = std::string(level);
        entry["event"] = std::string(event);
        if (fields.isObject()) {
            for (const auto& key : fields.getMemberNames())
                entry[key] = fields[key];
        }
        std::cout << Json::writeString(w, entry) << std::endl;
    }

    static void info(std::string_view event, const Json::Value& f = {}) noexcept { log("info", event, f); }
    static void error(std::string_view event, const Json::Value& f = {}) noexcept { log("error", event, f); }
    static void warn(std::string_view event, const Json::Value& f = {}) noexcept { log("warn", event, f); }
};
