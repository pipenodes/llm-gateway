#include "usage_tracking_plugin.h"
#include "../logger.h"
#include "../crypto.h"
#include <fstream>
#include <chrono>
#include <ctime>

namespace {

int64_t utc_day_start(int64_t epoch) {
    constexpr int64_t SECS_PER_DAY = 86400;
    return (epoch / SECS_PER_DAY) * SECS_PER_DAY;
}

int64_t utc_month_start(int64_t epoch) {
    std::time_t t = static_cast<std::time_t>(epoch);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &t);
#else
    gmtime_r(&t, &utc);
#endif
    utc.tm_mday = 1;
    utc.tm_hour = 0;
    utc.tm_min = 0;
    utc.tm_sec = 0;
#ifdef _WIN32
    return static_cast<int64_t>(_mkgmtime(&utc));
#else
    return static_cast<int64_t>(timegm(&utc));
#endif
}

UsageRecord json_to_record(const Json::Value& v) {
    UsageRecord r;
    r.prompt_tokens = v.get("prompt_tokens", 0).asInt64();
    r.completion_tokens = v.get("completion_tokens", 0).asInt64();
    r.total_tokens = v.get("total_tokens", 0).asInt64();
    r.request_count = v.get("request_count", 0).asInt64();
    r.period_start = v.get("period_start", 0).asInt64();
    return r;
}

Json::Value record_to_json(const UsageRecord& r) {
    Json::Value v;
    v["prompt_tokens"] = static_cast<Json::Int64>(r.prompt_tokens);
    v["completion_tokens"] = static_cast<Json::Int64>(r.completion_tokens);
    v["total_tokens"] = static_cast<Json::Int64>(r.total_tokens);
    v["request_count"] = static_cast<Json::Int64>(r.request_count);
    v["period_start"] = static_cast<Json::Int64>(r.period_start);
    return v;
}

QuotaPolicy json_to_quota(const Json::Value& v) {
    QuotaPolicy q;
    q.max_tokens_per_day = v.get("max_tokens_per_day", 0).asInt64();
    q.max_tokens_per_month = v.get("max_tokens_per_month", 0).asInt64();
    q.max_requests_per_day = v.get("max_requests_per_day", 0).asInt64();
    q.max_requests_per_month = v.get("max_requests_per_month", 0).asInt64();
    return q;
}

Json::Value quota_to_json(const QuotaPolicy& q) {
    Json::Value v;
    v["max_tokens_per_day"] = static_cast<Json::Int64>(q.max_tokens_per_day);
    v["max_tokens_per_month"] = static_cast<Json::Int64>(q.max_tokens_per_month);
    v["max_requests_per_day"] = static_cast<Json::Int64>(q.max_requests_per_day);
    v["max_requests_per_month"] = static_cast<Json::Int64>(q.max_requests_per_month);
    return v;
}

} // namespace

UsageTrackingPlugin::~UsageTrackingPlugin() {
    shutdown();
}

bool UsageTrackingPlugin::init(const Json::Value& config) {
    enabled_ = config.get("enabled", true).asBool();
    persist_path_ = config.get("persist_path", "usage.json").asString();
    flush_interval_seconds_ = config.get("flush_interval_seconds", 60).asInt();
    if (flush_interval_seconds_ < 1) flush_interval_seconds_ = 1;

    if (config.isMember("default_quota") && config["default_quota"].isObject()) {
        default_quota_ = json_to_quota(config["default_quota"]);
    }

    load();

    running_ = true;
    flush_thread_ = std::jthread([this] { flush_loop(); });

    Json::Value f;
    f["persist_path"] = persist_path_;
    f["flush_interval"] = flush_interval_seconds_;
    Logger::info("usage_tracking_init", f);
    return true;
}

void UsageTrackingPlugin::shutdown() {
    if (running_.exchange(false)) {
        flush_thread_.request_stop();
        if (flush_thread_.joinable()) flush_thread_.join();
        std::unique_lock lock(mtx_);
        save_unlocked();
    }
}

void UsageTrackingPlugin::on_request_completed(const core::AuditEntry& entry) {
    if (!enabled_) return;

    std::unique_lock lock(mtx_);

    auto& data = usage_[entry.key_alias.empty() ? "anonymous" : entry.key_alias];
    rotate_periods_unlocked(data);

    int64_t total = static_cast<int64_t>(entry.prompt_tokens) +
                    static_cast<int64_t>(entry.completion_tokens);

    data.daily.prompt_tokens += entry.prompt_tokens;
    data.daily.completion_tokens += entry.completion_tokens;
    data.daily.total_tokens += total;
    data.daily.request_count++;

    data.monthly.prompt_tokens += entry.prompt_tokens;
    data.monthly.completion_tokens += entry.completion_tokens;
    data.monthly.total_tokens += total;
    data.monthly.request_count++;

    auto& model_rec = data.by_model[entry.model.empty() ? "unknown" : entry.model];
    model_rec.total_tokens += total;
    model_rec.request_count++;

    dirty_ = true;
}

PluginResult UsageTrackingPlugin::before_request(Json::Value&, PluginRequestContext& ctx) {
    if (!enabled_)
        return PluginResult{.action = PluginAction::Continue};

    std::shared_lock lock(mtx_);
    auto it = usage_.find(ctx.key_alias.empty() ? "anonymous" : ctx.key_alias);
    if (it == usage_.end())
        return PluginResult{.action = PluginAction::Continue};

    const auto& data = it->second;
    if (!check_quota_unlocked(data)) {
        return PluginResult{
            .action = PluginAction::Block,
            .error_code = 429,
            .error_message = "Quota exceeded"
        };
    }
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult UsageTrackingPlugin::after_response(Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}

Json::Value UsageTrackingPlugin::stats() const {
    std::shared_lock lock(mtx_);
    Json::Value out;
    out["tracked_keys"] = static_cast<int>(usage_.size());
    out["enabled"] = enabled_;
    return out;
}

bool UsageTrackingPlugin::check_quota_unlocked(const KeyUsageData& data) const {
    const auto& q = (data.quota.max_tokens_per_day > 0 || data.quota.max_tokens_per_month > 0 ||
                     data.quota.max_requests_per_day > 0 || data.quota.max_requests_per_month > 0)
                    ? data.quota : default_quota_;

    if (q.max_tokens_per_day > 0 && data.daily.total_tokens >= q.max_tokens_per_day)
        return false;
    if (q.max_tokens_per_month > 0 && data.monthly.total_tokens >= q.max_tokens_per_month)
        return false;
    if (q.max_requests_per_day > 0 && data.daily.request_count >= q.max_requests_per_day)
        return false;
    if (q.max_requests_per_month > 0 && data.monthly.request_count >= q.max_requests_per_month)
        return false;
    return true;
}

void UsageTrackingPlugin::rotate_periods_unlocked(KeyUsageData& data) const {
    int64_t now = static_cast<int64_t>(crypto::epoch_seconds());
    int64_t current_day = utc_day_start(now);
    int64_t current_month = utc_month_start(now);

    if (data.daily.period_start < current_day) {
        data.daily = UsageRecord{};
        data.daily.period_start = current_day;
    }
    if (data.monthly.period_start < current_month) {
        data.monthly = UsageRecord{};
        data.monthly.period_start = current_month;
        data.by_model.clear();
    }
}

Json::Value UsageTrackingPlugin::get_usage_json(const std::string& alias) const {
    std::shared_lock lock(mtx_);
    auto it = usage_.find(alias);
    if (it == usage_.end()) {
        Json::Value out;
        out["error"] = "not_found";
        return out;
    }

    const auto& data = it->second;
    Json::Value out;
    out["alias"] = alias;
    out["daily"] = record_to_json(data.daily);
    out["monthly"] = record_to_json(data.monthly);

    Json::Value by_model(Json::objectValue);
    for (const auto& [model, rec] : data.by_model) {
        Json::Value m;
        m["total_tokens"] = static_cast<Json::Int64>(rec.total_tokens);
        m["request_count"] = static_cast<Json::Int64>(rec.request_count);
        by_model[model] = m;
    }
    out["by_model"] = by_model;

    const auto& q = (data.quota.max_tokens_per_day > 0 || data.quota.max_tokens_per_month > 0 ||
                     data.quota.max_requests_per_day > 0 || data.quota.max_requests_per_month > 0)
                    ? data.quota : default_quota_;
    Json::Value qj = quota_to_json(q);
    if (q.max_tokens_per_day > 0)
        qj["used_percentage_daily"] = static_cast<double>(data.daily.total_tokens) / q.max_tokens_per_day * 100.0;
    if (q.max_tokens_per_month > 0)
        qj["used_percentage_monthly"] = static_cast<double>(data.monthly.total_tokens) / q.max_tokens_per_month * 100.0;
    out["quota"] = qj;

    return out;
}

Json::Value UsageTrackingPlugin::get_all_usage_json() const {
    std::shared_lock lock(mtx_);
    Json::Value out(Json::objectValue);
    Json::Value keys(Json::arrayValue);
    int64_t total_tokens_all = 0;
    int64_t total_requests_all = 0;

    for (const auto& [alias, data] : usage_) {
        Json::Value entry;
        entry["alias"] = alias;
        entry["daily_tokens"] = static_cast<Json::Int64>(data.daily.total_tokens);
        entry["daily_requests"] = static_cast<Json::Int64>(data.daily.request_count);
        entry["monthly_tokens"] = static_cast<Json::Int64>(data.monthly.total_tokens);
        entry["monthly_requests"] = static_cast<Json::Int64>(data.monthly.request_count);
        keys.append(entry);
        total_tokens_all += data.monthly.total_tokens;
        total_requests_all += data.monthly.request_count;
    }

    out["keys"] = keys;
    out["total_monthly_tokens"] = static_cast<Json::Int64>(total_tokens_all);
    out["total_monthly_requests"] = static_cast<Json::Int64>(total_requests_all);
    out["tracked_keys"] = static_cast<int>(usage_.size());
    return out;
}

bool UsageTrackingPlugin::reset_usage(const std::string& alias) {
    std::unique_lock lock(mtx_);
    auto it = usage_.find(alias);
    if (it == usage_.end()) return false;
    usage_.erase(it);
    dirty_ = true;
    return true;
}

int64_t UsageTrackingPlugin::day_start(int64_t epoch) {
    return utc_day_start(epoch);
}

int64_t UsageTrackingPlugin::month_start(int64_t epoch) {
    return utc_month_start(epoch);
}

void UsageTrackingPlugin::flush() {
    std::shared_lock lock(mtx_);
    if (!dirty_) return;
    lock.unlock();

    std::unique_lock wlock(mtx_);
    save_unlocked();
    dirty_ = false;
}

void UsageTrackingPlugin::flush_loop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(flush_interval_seconds_));
        if (!running_) break;
        flush();
    }
}

void UsageTrackingPlugin::load() {
    if (persist_path_.empty()) return;
    std::ifstream file(persist_path_);
    if (!file.is_open()) return;

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    if (!Json::parseFromStream(builder, file, &root, &errs) || !root.isObject()) return;

    std::unique_lock lock(mtx_);
    usage_.clear();

    for (const auto& alias : root.getMemberNames()) {
        const auto& v = root[alias];
        KeyUsageData data;
        if (v.isMember("daily")) data.daily = json_to_record(v["daily"]);
        if (v.isMember("monthly")) data.monthly = json_to_record(v["monthly"]);
        if (v.isMember("quota")) data.quota = json_to_quota(v["quota"]);
        if (v.isMember("by_model") && v["by_model"].isObject()) {
            for (const auto& model : v["by_model"].getMemberNames()) {
                data.by_model[model] = json_to_record(v["by_model"][model]);
            }
        }
        rotate_periods_unlocked(data);
        usage_[alias] = std::move(data);
    }

    Json::Value f;
    f["keys_loaded"] = static_cast<int>(usage_.size());
    Logger::info("usage_data_loaded", f);
}

void UsageTrackingPlugin::save_unlocked() const {
    if (persist_path_.empty()) return;

    Json::StreamWriterBuilder w;
    w["indentation"] = "  ";
    Json::Value root(Json::objectValue);

    for (const auto& [alias, data] : usage_) {
        Json::Value entry;
        entry["daily"] = record_to_json(data.daily);
        entry["monthly"] = record_to_json(data.monthly);
        entry["quota"] = quota_to_json(data.quota);

        Json::Value by_model(Json::objectValue);
        for (const auto& [model, rec] : data.by_model) {
            by_model[model] = record_to_json(rec);
        }
        entry["by_model"] = by_model;
        root[alias] = entry;
    }

    std::ofstream file(persist_path_);
    if (file.is_open())
        file << Json::writeString(w, root) << '\n';
}
