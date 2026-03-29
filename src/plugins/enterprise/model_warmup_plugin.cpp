#include "model_warmup_plugin.h"
#include "../logger.h"
#include "../crypto.h"
#include "../httplib.h"
#include <chrono>
#include <ctime>
#include <sstream>

ModelWarmupPlugin::~ModelWarmupPlugin() {
    shutdown();
}

bool ModelWarmupPlugin::init(const Json::Value& config) {
    ollama_host_ = config.get("ollama_host", "localhost").asString();
    ollama_port_ = config.get("ollama_port", 11434).asInt();
    warmup_prompt_ = config.get("warmup_prompt", "Hi").asString();
    warmup_max_tokens_ = config.get("warmup_max_tokens", 1).asInt();
    startup_warmup_ = config.get("startup_warmup", true).asBool();

    if (config.isMember("models") && config["models"].isArray()) {
        for (const auto& m : config["models"]) {
            WarmupModel wm;
            if (m.isString()) {
                wm.name = m.asString();
            } else if (m.isObject()) {
                wm.name = m.get("name", "").asString();
                wm.interval_seconds = m.get("interval_seconds", 240).asInt();
                wm.always_loaded = m.get("always_loaded", false).asBool();
                wm.schedule = m.get("schedule", "").asString();
            }
            if (!wm.name.empty()) {
                status_[wm.name] = ModelStatus{.name = wm.name};
                models_.push_back(std::move(wm));
            }
        }
    }

    if (models_.empty()) {
        Logger::info("model_warmup_no_models");
        return true;
    }

    running_ = true;

    if (startup_warmup_) {
        for (const auto& m : models_) {
            warmup_model(m);
        }
    }

    warmup_thread_ = std::jthread([this] { warmup_loop(); });

    Json::Value f;
    f["models"] = static_cast<int>(models_.size());
    f["startup_warmup"] = startup_warmup_;
    Logger::info("model_warmup_init", f);
    return true;
}

void ModelWarmupPlugin::shutdown() {
    if (running_.exchange(false)) {
        if (warmup_thread_.joinable()) warmup_thread_.join();
    }
}

void ModelWarmupPlugin::warmup_model(const WarmupModel& model) {
    auto start = std::chrono::steady_clock::now();

    try {
        httplib::Client cli(ollama_host_, ollama_port_);
        cli.set_connection_timeout(5);
        cli.set_read_timeout(30);

        Json::Value body;
        body["model"] = model.name;
        body["stream"] = false;
        Json::Value msgs(Json::arrayValue);
        Json::Value msg;
        msg["role"] = "user";
        msg["content"] = warmup_prompt_;
        msgs.append(msg);
        body["messages"] = msgs;

        Json::Value options;
        options["num_predict"] = warmup_max_tokens_;
        body["options"] = options;

        Json::StreamWriterBuilder w;
        w["indentation"] = "";
        std::string req_body = Json::writeString(w, body);

        auto result = cli.Post("/api/chat", req_body, "application/json");

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

        std::lock_guard lock(mtx_);
        auto& s = status_[model.name];
        s.last_warmup = static_cast<int64_t>(crypto::epoch_seconds());
        s.warmup_count++;

        if (result && result->status == 200) {
            s.loaded = true;
            s.total_load_time_ms += elapsed;
            s.avg_load_time_ms = s.total_load_time_ms / s.warmup_count;

            Json::Value f;
            f["model"] = model.name;
            f["elapsed_ms"] = static_cast<Json::Int64>(elapsed);
            Logger::info("model_warmup_ok", f);
        } else {
            s.loaded = false;
            s.cold_start_count++;

            Json::Value f;
            f["model"] = model.name;
            f["status"] = result ? result->status : 0;
            Logger::warn("model_warmup_failed", f);
        }
    } catch (const std::exception& e) {
        std::lock_guard lock(mtx_);
        auto& s = status_[model.name];
        s.loaded = false;
        s.cold_start_count++;

        Json::Value f;
        f["model"] = model.name;
        f["error"] = e.what();
        Logger::warn("model_warmup_error", f);
    }
}

bool ModelWarmupPlugin::is_within_schedule(const std::string& schedule) const {
    if (schedule.empty()) return true;

    auto dash = schedule.find('-');
    if (dash == std::string::npos) return true;

    std::string start_str = schedule.substr(0, dash);
    std::string end_str = schedule.substr(dash + 1);

    int start_h = 0, start_m = 0, end_h = 0, end_m = 0;
    auto colon1 = start_str.find(':');
    auto colon2 = end_str.find(':');
    if (colon1 == std::string::npos || colon2 == std::string::npos) return true;

    try {
        start_h = std::stoi(start_str.substr(0, colon1));
        start_m = std::stoi(start_str.substr(colon1 + 1));
        end_h = std::stoi(end_str.substr(0, colon2));
        end_m = std::stoi(end_str.substr(colon2 + 1));
    } catch (...) { return true; }

    auto now_epoch = static_cast<std::time_t>(crypto::epoch_seconds());
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &now_epoch);
#else
    gmtime_r(&now_epoch, &utc);
#endif

    int current_minutes = utc.tm_hour * 60 + utc.tm_min;
    int start_minutes = start_h * 60 + start_m;
    int end_minutes = end_h * 60 + end_m;

    if (start_minutes <= end_minutes)
        return current_minutes >= start_minutes && current_minutes <= end_minutes;
    return current_minutes >= start_minutes || current_minutes <= end_minutes;
}

void ModelWarmupPlugin::warmup_loop() {
    while (running_) {
        int min_interval = 240;
        for (const auto& m : models_)
            min_interval = std::min(min_interval, m.interval_seconds);

        std::this_thread::sleep_for(std::chrono::seconds(min_interval));
        if (!running_) break;

        int64_t now = static_cast<int64_t>(crypto::epoch_seconds());

        for (const auto& m : models_) {
            if (!running_) break;
            if (!is_within_schedule(m.schedule)) continue;

            bool needs_warmup = false;
            {
                std::lock_guard lock(mtx_);
                auto it = status_.find(m.name);
                if (it != status_.end()) {
                    needs_warmup = (now - it->second.last_warmup) >= m.interval_seconds;
                } else {
                    needs_warmup = true;
                }
            }

            if (needs_warmup)
                warmup_model(m);
        }
    }
}

PluginResult ModelWarmupPlugin::before_request(Json::Value&, PluginRequestContext& ctx) {
    if (!ctx.model.empty()) {
        std::lock_guard lock(mtx_);
        auto it = status_.find(ctx.model);
        if (it != status_.end())
            it->second.last_used = static_cast<int64_t>(crypto::epoch_seconds());
    }
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult ModelWarmupPlugin::after_response(Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}

Json::Value ModelWarmupPlugin::stats() const {
    std::lock_guard lock(mtx_);
    Json::Value out(Json::objectValue);
    Json::Value models(Json::objectValue);

    for (const auto& [name, s] : status_) {
        Json::Value m;
        m["loaded"] = s.loaded;
        m["last_warmup"] = static_cast<Json::Int64>(s.last_warmup);
        m["last_used"] = static_cast<Json::Int64>(s.last_used);
        m["warmup_count"] = s.warmup_count;
        m["cold_start_count"] = s.cold_start_count;
        m["avg_load_time_ms"] = s.avg_load_time_ms;
        models[name] = m;
    }

    out["models"] = models;
    return out;
}
