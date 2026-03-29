#include "cost_controller_plugin.h"
#include "../logger.h"
#include "../crypto.h"
#include <fstream>
#include <ctime>
#include <sstream>
#include <iomanip>

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

int estimate_input_tokens(const Json::Value& body) {
    int tokens = 0;
    if (body.isMember("messages") && body["messages"].isArray()) {
        for (const auto& msg : body["messages"]) {
            std::string content = msg.get("content", "").asString();
            tokens += static_cast<int>(content.size()) / 4 + 4;
        }
    } else if (body.isMember("prompt")) {
        tokens = static_cast<int>(body["prompt"].asString().size()) / 4;
    }
    return std::max(tokens, 1);
}

} // namespace

CostControllerPlugin::~CostControllerPlugin() {
    shutdown();
}

bool CostControllerPlugin::init(const Json::Value& config) {
    default_monthly_limit_ = config.get("default_monthly_limit_usd", 0.0).asDouble();
    default_daily_limit_ = config.get("default_daily_limit_usd", 0.0).asDouble();
    fallback_model_ = config.get("fallback_model", "").asString();
    warning_threshold_ = config.get("warning_threshold", 0.8).asDouble();
    add_cost_header_ = config.get("add_cost_header", true).asBool();
    persist_path_ = config.get("persist_path", "costs.json").asString();
    flush_interval_seconds_ = config.get("flush_interval_seconds", 60).asInt();

    if (config.isMember("pricing") && config["pricing"].isObject()) {
        for (const auto& model : config["pricing"].getMemberNames()) {
            const auto& p = config["pricing"][model];
            pricing_[model] = {
                p.get("input_per_1k", 0.0).asDouble(),
                p.get("output_per_1k", 0.0).asDouble()
            };
        }
    }

    if (config.isMember("budgets") && config["budgets"].isObject()) {
        for (const auto& alias : config["budgets"].getMemberNames()) {
            const auto& b = config["budgets"][alias];
            CostBudget budget;
            budget.monthly_limit_usd = b.get("monthly_limit_usd", default_monthly_limit_).asDouble();
            budget.daily_limit_usd = b.get("daily_limit_usd", default_daily_limit_).asDouble();
            budgets_[alias] = budget;
        }
    }

    load();

    running_ = true;
    flush_thread_ = std::jthread([this] { flush_loop(); });

    Json::Value f;
    f["models_priced"] = static_cast<int>(pricing_.size());
    f["budgets"] = static_cast<int>(budgets_.size());
    Logger::info("cost_controller_init", f);
    return true;
}

void CostControllerPlugin::shutdown() {
    if (running_.exchange(false)) {
        if (flush_thread_.joinable()) flush_thread_.join();
        std::shared_lock lock(mtx_);
        save_unlocked();
    }
}

double CostControllerPlugin::estimate_cost(const std::string& model, int estimated_tokens) const {
    auto it = pricing_.find(model);
    if (it == pricing_.end()) return 0.001;
    return (static_cast<double>(estimated_tokens) / 1000.0) * it->second.input_per_1k;
}

void CostControllerPlugin::record_cost(const std::string& key_alias, const std::string& model,
                                         int input_tokens, int output_tokens) {
    std::unique_lock lock(mtx_);
    auto& budget = budgets_[key_alias.empty() ? "default" : key_alias];
    rotate_periods(budget);

    double cost = 0.0;
    auto it = pricing_.find(model);
    if (it != pricing_.end()) {
        cost = (static_cast<double>(input_tokens) / 1000.0) * it->second.input_per_1k
             + (static_cast<double>(output_tokens) / 1000.0) * it->second.output_per_1k;
    }

    budget.spent_daily_usd += cost;
    budget.spent_monthly_usd += cost;

    auto& mc = budget.by_model[model];
    mc.requests++;
    mc.cost_usd += cost;

    dirty_ = true;
}

void CostControllerPlugin::rotate_periods(CostBudget& b) const {
    int64_t now = static_cast<int64_t>(crypto::epoch_seconds());
    int64_t current_day = utc_day_start(now);
    int64_t current_month = utc_month_start(now);

    if (b.daily_period_start < current_day) {
        b.spent_daily_usd = 0.0;
        b.daily_period_start = current_day;
    }
    if (b.monthly_period_start < current_month) {
        b.spent_monthly_usd = 0.0;
        b.monthly_period_start = current_month;
        b.by_model.clear();
    }
}

PluginResult CostControllerPlugin::before_request(Json::Value& body, PluginRequestContext& ctx) {
    std::string alias = ctx.key_alias.empty() ? "default" : ctx.key_alias;
    std::string model = ctx.model;
    int estimated_tokens = estimate_input_tokens(body);
    double est_cost = estimate_cost(model, estimated_tokens);

    {
        std::shared_lock lock(mtx_);
        auto it = budgets_.find(alias);
        if (it != budgets_.end()) {
            const auto& b = it->second;
            if (b.daily_limit_usd > 0 && b.spent_daily_usd + est_cost > b.daily_limit_usd) {
                return PluginResult{.action = PluginAction::Block, .error_code = 402,
                    .error_message = "Daily budget exceeded"};
            }
            if (b.monthly_limit_usd > 0 && b.spent_monthly_usd + est_cost > b.monthly_limit_usd) {
                return PluginResult{.action = PluginAction::Block, .error_code = 402,
                    .error_message = "Monthly budget exceeded"};
            }

            if (add_cost_header_) {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(6) << est_cost;
                ctx.metadata["X-Cost-Estimate"] = oss.str();

                double remaining = 0;
                if (b.monthly_limit_usd > 0)
                    remaining = b.monthly_limit_usd - b.spent_monthly_usd;
                std::ostringstream oss2;
                oss2 << std::fixed << std::setprecision(4) << remaining;
                ctx.metadata["X-Budget-Remaining"] = oss2.str();
            }
        } else if (default_monthly_limit_ > 0 || default_daily_limit_ > 0) {
            if (add_cost_header_) {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(6) << est_cost;
                ctx.metadata["X-Cost-Estimate"] = oss.str();
            }
        }
    }

    return PluginResult{.action = PluginAction::Continue};
}

PluginResult CostControllerPlugin::after_response(Json::Value& response, PluginRequestContext& ctx) {
    int input_tokens = 0;
    int output_tokens = 0;
    if (response.isMember("usage")) {
        input_tokens = response["usage"].get("prompt_tokens", 0).asInt();
        output_tokens = response["usage"].get("completion_tokens", 0).asInt();
    }
    if (input_tokens > 0 || output_tokens > 0)
        record_cost(ctx.key_alias, ctx.model, input_tokens, output_tokens);
    return PluginResult{.action = PluginAction::Continue};
}

void CostControllerPlugin::on_request_completed(const core::AuditEntry& entry) {
    if (entry.prompt_tokens > 0 || entry.completion_tokens > 0)
        record_cost(entry.key_alias, entry.model, entry.prompt_tokens, entry.completion_tokens);
}

Json::Value CostControllerPlugin::stats() const {
    std::shared_lock lock(mtx_);
    Json::Value out;
    out["tracked_keys"] = static_cast<int>(budgets_.size());
    out["models_priced"] = static_cast<int>(pricing_.size());
    return out;
}

Json::Value CostControllerPlugin::get_all_costs() const {
    std::shared_lock lock(mtx_);
    Json::Value out(Json::objectValue);
    Json::Value keys(Json::arrayValue);
    double total_spent = 0;

    for (const auto& [alias, b] : budgets_) {
        Json::Value entry;
        entry["alias"] = alias;
        entry["spent_monthly_usd"] = b.spent_monthly_usd;
        entry["spent_daily_usd"] = b.spent_daily_usd;
        entry["monthly_limit_usd"] = b.monthly_limit_usd;
        entry["daily_limit_usd"] = b.daily_limit_usd;
        keys.append(entry);
        total_spent += b.spent_monthly_usd;
    }

    out["keys"] = keys;
    out["total_monthly_spent_usd"] = total_spent;
    return out;
}

Json::Value CostControllerPlugin::get_key_costs(const std::string& alias) const {
    std::shared_lock lock(mtx_);
    auto it = budgets_.find(alias);
    if (it == budgets_.end()) return Json::Value();

    const auto& b = it->second;
    Json::Value out;
    out["alias"] = alias;

    Json::Value budget;
    budget["monthly_limit_usd"] = b.monthly_limit_usd;
    budget["daily_limit_usd"] = b.daily_limit_usd;
    budget["spent_monthly_usd"] = b.spent_monthly_usd;
    budget["spent_daily_usd"] = b.spent_daily_usd;
    budget["remaining_monthly_usd"] = std::max(0.0, b.monthly_limit_usd - b.spent_monthly_usd);
    if (b.monthly_limit_usd > 0)
        budget["used_percentage"] = (b.spent_monthly_usd / b.monthly_limit_usd) * 100.0;
    out["budget"] = budget;

    Json::Value by_model(Json::objectValue);
    for (const auto& [model, mc] : b.by_model) {
        Json::Value m;
        m["requests"] = static_cast<Json::Int64>(mc.requests);
        m["cost_usd"] = mc.cost_usd;
        by_model[model] = m;
    }
    out["by_model"] = by_model;
    return out;
}

void CostControllerPlugin::set_budget(const std::string& alias, double monthly, double daily) {
    std::unique_lock lock(mtx_);
    auto& b = budgets_[alias];
    b.monthly_limit_usd = monthly;
    b.daily_limit_usd = daily;
    dirty_ = true;
}

void CostControllerPlugin::load() {
    if (persist_path_.empty()) return;
    std::ifstream file(persist_path_);
    if (!file.is_open()) return;

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    if (!Json::parseFromStream(builder, file, &root, &errs) || !root.isObject()) return;

    std::unique_lock lock(mtx_);
    for (const auto& alias : root.getMemberNames()) {
        const auto& v = root[alias];
        auto& b = budgets_[alias];
        b.monthly_limit_usd = v.get("monthly_limit_usd", b.monthly_limit_usd).asDouble();
        b.daily_limit_usd = v.get("daily_limit_usd", b.daily_limit_usd).asDouble();
        b.spent_monthly_usd = v.get("spent_monthly_usd", 0.0).asDouble();
        b.spent_daily_usd = v.get("spent_daily_usd", 0.0).asDouble();
        b.daily_period_start = v.get("daily_period_start", 0).asInt64();
        b.monthly_period_start = v.get("monthly_period_start", 0).asInt64();

        if (v.isMember("by_model") && v["by_model"].isObject()) {
            for (const auto& model : v["by_model"].getMemberNames()) {
                auto& mc = b.by_model[model];
                mc.requests = v["by_model"][model].get("requests", 0).asInt64();
                mc.cost_usd = v["by_model"][model].get("cost_usd", 0.0).asDouble();
            }
        }

        rotate_periods(b);
    }
}

void CostControllerPlugin::save_unlocked() const {
    if (persist_path_.empty()) return;
    Json::StreamWriterBuilder w;
    w["indentation"] = "  ";
    Json::Value root(Json::objectValue);

    for (const auto& [alias, b] : budgets_) {
        Json::Value entry;
        entry["monthly_limit_usd"] = b.monthly_limit_usd;
        entry["daily_limit_usd"] = b.daily_limit_usd;
        entry["spent_monthly_usd"] = b.spent_monthly_usd;
        entry["spent_daily_usd"] = b.spent_daily_usd;
        entry["daily_period_start"] = static_cast<Json::Int64>(b.daily_period_start);
        entry["monthly_period_start"] = static_cast<Json::Int64>(b.monthly_period_start);

        Json::Value by_model(Json::objectValue);
        for (const auto& [model, mc] : b.by_model) {
            Json::Value m;
            m["requests"] = static_cast<Json::Int64>(mc.requests);
            m["cost_usd"] = mc.cost_usd;
            by_model[model] = m;
        }
        entry["by_model"] = by_model;
        root[alias] = entry;
    }

    std::ofstream file(persist_path_);
    if (file.is_open())
        file << Json::writeString(w, root) << '\n';
}

void CostControllerPlugin::flush_loop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(flush_interval_seconds_));
        if (!running_) break;
        if (dirty_.exchange(false)) {
            std::shared_lock lock(mtx_);
            save_unlocked();
        }
    }
}
