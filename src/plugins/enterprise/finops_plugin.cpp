#include "finops_plugin.h"
#include "../../tenant_ctx.h"
#include "../logger.h"
#include "../crypto.h"
#include "../httplib.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <algorithm>
#include <ctime>

// ── Time helpers ──────────────────────────────────────────────────────────────

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
    utc.tm_mday = 1; utc.tm_hour = 0; utc.tm_min = 0; utc.tm_sec = 0;
#ifdef _WIN32
    return static_cast<int64_t>(_mkgmtime(&utc));
#else
    return static_cast<int64_t>(timegm(&utc));
#endif
}

bool parse_webhook_url(const std::string& url, std::string& host, int& port,
                       std::string& path) {
    if (url.empty()) return false;
    port = 443;
    size_t start = 8; // https://
    if (url.find("http://") == 0) { port = 80; start = 7; }
    else if (url.find("https://") != 0) return false;
    size_t slash = url.find('/', start);
    std::string host_port = (slash == std::string::npos) ? url.substr(start)
                                                          : url.substr(start, slash - start);
    path = (slash != std::string::npos) ? url.substr(slash) : "/";
    size_t colon = host_port.rfind(':');
    if (colon != std::string::npos) {
        host = host_port.substr(0, colon);
        try { port = std::stoi(host_port.substr(colon + 1)); } catch (...) {}
    } else {
        host = host_port;
    }
    return !host.empty();
}

} // namespace

// ── Init ──────────────────────────────────────────────────────────────────────

bool FinOpsPlugin::init(const Json::Value& config) {
    persist_path_           = config.get("persist_path", "data/finops-costs.json").asString();
    flush_interval_seconds_ = config.get("flush_interval_seconds", 60).asInt();
    alert_webhook_url_      = config.get("alert_webhook_url", "").asString();

    if (config.isMember("pricing") && config["pricing"].isObject()) {
        for (const auto& model : config["pricing"].getMemberNames()) {
            const auto& p = config["pricing"][model];
            pricing_[model] = {
                p.get("input_per_1k", 0.0).asDouble(),
                p.get("output_per_1k", 0.0).asDouble()
            };
        }
    }

    auto load_budgets = [this](const Json::Value& node, const std::string& prefix) {
        if (!node.isObject()) return;
        for (const auto& key : node.getMemberNames()) {
            const auto& b = node[key];
            std::string full_key = prefix.empty() ? key : prefix + ":" + key;
            auto& budget = budgets_[full_key];
            budget.monthly_limit_usd = b.get("monthly_limit_usd", 0.0).asDouble();
            budget.daily_limit_usd   = b.get("daily_limit_usd", 0.0).asDouble();
        }
    };
    load_budgets(config["tenant_budgets"], "");
    load_budgets(config["app_budgets"],    "");
    load_budgets(config["key_budgets"],    "");

    load();

    running_ = true;
    flush_thread_ = std::jthread([this] { flush_loop(); });

    Json::Value f;
    f["models_priced"] = static_cast<int>(pricing_.size());
    f["budgets"]       = static_cast<int>(budgets_.size());
    Logger::info("finops_init", f);
    return true;
}

void FinOpsPlugin::shutdown() {
    if (running_.exchange(false)) {
        if (flush_thread_.joinable()) flush_thread_.join();
        std::shared_lock lock(mtx_);
        save_unlocked();
    }
}

// ── Cost computation ──────────────────────────────────────────────────────────

double FinOpsPlugin::compute_cost(const std::string& model,
                                   int input_tokens, int output_tokens) const {
    auto it = pricing_.find(model);
    if (it == pricing_.end()) return 0.0;
    return (static_cast<double>(input_tokens)  / 1000.0) * it->second.input_per_1k
         + (static_cast<double>(output_tokens) / 1000.0) * it->second.output_per_1k;
}

// ── Period rotation ───────────────────────────────────────────────────────────

void FinOpsPlugin::rotate_period(FinOpsBudget& b) const {
    int64_t now          = crypto::epoch_seconds();
    int64_t current_day  = utc_day_start(now);
    int64_t current_mon  = utc_month_start(now);
    if (b.daily_period_start < current_day) {
        b.spent_daily_usd    = 0.0;
        b.daily_period_start = current_day;
    }
    if (b.monthly_period_start < current_mon) {
        b.spent_monthly_usd    = 0.0;
        b.monthly_period_start = current_mon;
        b.alert_50_sent = b.alert_80_sent = b.alert_100_sent = false;
    }
}

// ── Budget check (before_request) ────────────────────────────────────────────

bool FinOpsPlugin::is_over_budget(const std::string& tenant, const std::string& app,
                                   const std::string& client, double est_cost) const {
    auto check = [&](const std::string& key) -> bool {
        auto it = budgets_.find(key);
        if (it == budgets_.end()) return false;
        const auto& b = it->second;
        if (b.monthly_limit_usd > 0 && (b.spent_monthly_usd + est_cost) > b.monthly_limit_usd)
            return true;
        if (b.daily_limit_usd > 0 && (b.spent_daily_usd + est_cost) > b.daily_limit_usd)
            return true;
        return false;
    };
    // Check all three levels: tenant > tenant:app > tenant:app:client
    return check(tenant)
        || check(tenant + ":" + app)
        || check(tenant + ":" + app + ":" + client);
}

// ── Record usage (after_response) ────────────────────────────────────────────

void FinOpsPlugin::record_usage(const std::string& tenant, const std::string& app,
                                 const std::string& client, const std::string& model,
                                 int input_tokens, int output_tokens) {
    double cost = compute_cost(model, input_tokens, output_tokens);
    std::unique_lock lock(mtx_);

    auto update_node = [&](const std::string& key) {
        auto& node = cost_nodes_[key];
        node.requests++;
        node.input_tokens  += input_tokens;
        node.output_tokens += output_tokens;
        node.cost_usd      += cost;
        node.by_model[model] += cost;
    };

    update_node(tenant);
    update_node(tenant + ":" + app);
    update_node(tenant + ":" + app + ":" + client);

    // Update budgets and check alerts
    auto update_budget = [&](const std::string& key) {
        auto it = budgets_.find(key);
        if (it == budgets_.end()) return;
        rotate_period(it->second);
        it->second.spent_monthly_usd += cost;
        it->second.spent_daily_usd   += cost;
        check_and_alert(key, it->second);
    };
    update_budget(tenant);
    update_budget(tenant + ":" + app);
    update_budget(tenant + ":" + app + ":" + client);

    dirty_ = true;
}

// ── Alert webhooks ────────────────────────────────────────────────────────────

void FinOpsPlugin::check_and_alert(const std::string& key, FinOpsBudget& b) {
    if (b.monthly_limit_usd <= 0 || alert_webhook_url_.empty()) return;
    double pct = b.spent_monthly_usd / b.monthly_limit_usd;
    int threshold = 0;
    if      (pct >= 1.0 && !b.alert_100_sent) { threshold = 100; b.alert_100_sent = true; }
    else if (pct >= 0.8 && !b.alert_80_sent)  { threshold = 80;  b.alert_80_sent  = true; }
    else if (pct >= 0.5 && !b.alert_50_sent)  { threshold = 50;  b.alert_50_sent  = true; }
    if (threshold == 0) return;

    Json::Value payload;
    payload["event"]       = "finops_budget_alert";
    payload["key"]         = key;
    payload["threshold"]   = threshold;
    payload["spent_usd"]   = b.spent_monthly_usd;
    payload["limit_usd"]   = b.monthly_limit_usd;
    payload["timestamp"]   = static_cast<Json::Int64>(crypto::epoch_seconds());

    Json::Value lf;
    lf["key"] = key; lf["threshold"] = threshold;
    Logger::warn("finops_budget_alert", lf);

    send_webhook_async(alert_webhook_url_, payload);
}

void FinOpsPlugin::send_webhook_async(const std::string& url,
                                       const Json::Value& payload) const {
    thread_local Json::StreamWriterBuilder w = [] {
        Json::StreamWriterBuilder b; b["indentation"] = ""; return b;
    }();
    std::string body_str = Json::writeString(w, payload);

    std::thread([url, body_str]() {
        try {
            std::string host, path;
            int port = 443;
            if (!parse_webhook_url(url, host, port, path)) return;
            httplib::Client cli(host, port);
            cli.set_connection_timeout(5);
            cli.set_read_timeout(10);
            cli.Post(path, body_str, "application/json");
        } catch (...) {}
    }).detach();
}

// ── Plugin hooks ──────────────────────────────────────────────────────────────

PluginResult FinOpsPlugin::before_request(Json::Value& body, PluginRequestContext& ctx) {
    total_requests_.fetch_add(1, std::memory_order_relaxed);

    std::string tenant = ctx_tenant(ctx);
    std::string app    = ctx_app(ctx);
    std::string client = ctx_client(ctx);

    // Estimate cost for budget pre-check (rough: assume 256 tokens input)
    double est_cost = compute_cost(ctx.model, 256, 0);

    {
        std::shared_lock lock(mtx_);
        // Rotate periods before checking
        auto check_with_rotation = [&](const std::string& key) -> bool {
            auto it = budgets_.find(key);
            if (it == budgets_.end()) return false;
            const auto& b = it->second;
            if (b.monthly_limit_usd > 0 &&
                (b.spent_monthly_usd + est_cost) > b.monthly_limit_usd)
                return true;
            if (b.daily_limit_usd > 0 &&
                (b.spent_daily_usd + est_cost) > b.daily_limit_usd)
                return true;
            return false;
        };

        bool over = check_with_rotation(tenant)
                 || check_with_rotation(tenant + ":" + app)
                 || check_with_rotation(tenant + ":" + app + ":" + client);

        if (over) {
            budget_blocks_.fetch_add(1, std::memory_order_relaxed);
            return PluginResult{.action = PluginAction::Block, .error_code = 402,
                .error_message = "Budget exceeded for tenant"};
        }
    }

    return PluginResult{.action = PluginAction::Continue};
}

PluginResult FinOpsPlugin::after_response(Json::Value& response, PluginRequestContext& ctx) {
    int input_tokens  = 0;
    int output_tokens = 0;
    if (response.isMember("usage")) {
        input_tokens  = response["usage"].get("prompt_tokens", 0).asInt();
        output_tokens = response["usage"].get("completion_tokens", 0).asInt();
    }
    if (input_tokens > 0 || output_tokens > 0)
        record_usage(ctx_tenant(ctx), ctx_app(ctx), ctx_client(ctx),
                     ctx.model, input_tokens, output_tokens);
    return PluginResult{.action = PluginAction::Continue};
}

// ── Admin helpers ─────────────────────────────────────────────────────────────

Json::Value FinOpsPlugin::get_costs(const std::string& tenant_id) const {
    std::shared_lock lock(mtx_);
    Json::Value out(Json::objectValue);
    for (const auto& [key, node] : cost_nodes_) {
        if (!tenant_id.empty() && key.find(tenant_id) != 0) continue;
        Json::Value entry;
        entry["requests"]      = static_cast<Json::Int64>(node.requests);
        entry["input_tokens"]  = static_cast<Json::Int64>(node.input_tokens);
        entry["output_tokens"] = static_cast<Json::Int64>(node.output_tokens);
        entry["cost_usd"]      = node.cost_usd;
        Json::Value by_model(Json::objectValue);
        for (const auto& [m, c] : node.by_model) by_model[m] = c;
        entry["by_model"] = by_model;
        out[key] = entry;
    }
    return out;
}

Json::Value FinOpsPlugin::get_budget(const std::string& tenant_id) const {
    std::shared_lock lock(mtx_);
    Json::Value out(Json::objectValue);
    for (const auto& [key, b] : budgets_) {
        if (!tenant_id.empty() && key != tenant_id) continue;
        Json::Value entry;
        entry["monthly_limit_usd"]  = b.monthly_limit_usd;
        entry["daily_limit_usd"]    = b.daily_limit_usd;
        entry["spent_monthly_usd"]  = b.spent_monthly_usd;
        entry["spent_daily_usd"]    = b.spent_daily_usd;
        if (b.monthly_limit_usd > 0)
            entry["used_pct"] = (b.spent_monthly_usd / b.monthly_limit_usd) * 100.0;
        out[key] = entry;
    }
    return out;
}

void FinOpsPlugin::set_tenant_budget(const std::string& tenant_id,
                                      double monthly, double daily) {
    std::unique_lock lock(mtx_);
    auto& b = budgets_[tenant_id];
    b.monthly_limit_usd = monthly;
    b.daily_limit_usd   = daily;
    dirty_ = true;
}

std::string FinOpsPlugin::export_csv() const {
    std::shared_lock lock(mtx_);
    std::ostringstream csv;
    csv << "key,requests,input_tokens,output_tokens,cost_usd\n";
    for (const auto& [key, node] : cost_nodes_) {
        csv << key << ','
            << node.requests      << ','
            << node.input_tokens  << ','
            << node.output_tokens << ','
            << std::fixed << std::setprecision(6) << node.cost_usd << '\n';
    }
    return csv.str();
}

// ── Stats ─────────────────────────────────────────────────────────────────────

Json::Value FinOpsPlugin::stats() const {
    Json::Value out;
    out["total_requests"] = static_cast<Json::Int64>(total_requests_.load());
    out["budget_blocks"]  = static_cast<Json::Int64>(budget_blocks_.load());
    out["models_priced"]  = static_cast<int>(pricing_.size());
    {
        std::shared_lock lock(mtx_);
        out["cost_nodes"] = static_cast<int>(cost_nodes_.size());
        out["budgets"]    = static_cast<int>(budgets_.size());
    }
    return out;
}

// ── Persistence ───────────────────────────────────────────────────────────────

void FinOpsPlugin::load() {
    if (persist_path_.empty()) return;
    std::ifstream file(persist_path_);
    if (!file.is_open()) return;

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    if (!Json::parseFromStream(builder, file, &root, &errs) || !root.isObject()) return;

    std::unique_lock lock(mtx_);
    if (root.isMember("nodes") && root["nodes"].isObject()) {
        for (const auto& key : root["nodes"].getMemberNames()) {
            const auto& v = root["nodes"][key];
            auto& node = cost_nodes_[key];
            node.requests      = v.get("requests", 0).asInt64();
            node.input_tokens  = v.get("input_tokens", 0).asInt64();
            node.output_tokens = v.get("output_tokens", 0).asInt64();
            node.cost_usd      = v.get("cost_usd", 0.0).asDouble();
        }
    }
    if (root.isMember("budgets") && root["budgets"].isObject()) {
        for (const auto& key : root["budgets"].getMemberNames()) {
            const auto& v = root["budgets"][key];
            auto& b = budgets_[key];
            b.spent_monthly_usd   = v.get("spent_monthly_usd", 0.0).asDouble();
            b.spent_daily_usd     = v.get("spent_daily_usd", 0.0).asDouble();
            b.daily_period_start  = v.get("daily_period_start", 0).asInt64();
            b.monthly_period_start= v.get("monthly_period_start", 0).asInt64();
            rotate_period(b);
        }
    }
}

void FinOpsPlugin::save_unlocked() const {
    if (persist_path_.empty()) return;
    Json::Value root(Json::objectValue);

    Json::Value nodes(Json::objectValue);
    for (const auto& [key, node] : cost_nodes_) {
        Json::Value v;
        v["requests"]      = static_cast<Json::Int64>(node.requests);
        v["input_tokens"]  = static_cast<Json::Int64>(node.input_tokens);
        v["output_tokens"] = static_cast<Json::Int64>(node.output_tokens);
        v["cost_usd"]      = node.cost_usd;
        nodes[key] = v;
    }
    root["nodes"] = nodes;

    Json::Value budgets_json(Json::objectValue);
    for (const auto& [key, b] : budgets_) {
        Json::Value v;
        v["spent_monthly_usd"]    = b.spent_monthly_usd;
        v["spent_daily_usd"]      = b.spent_daily_usd;
        v["daily_period_start"]   = static_cast<Json::Int64>(b.daily_period_start);
        v["monthly_period_start"] = static_cast<Json::Int64>(b.monthly_period_start);
        budgets_json[key] = v;
    }
    root["budgets"] = budgets_json;

    Json::StreamWriterBuilder w;
    w["indentation"] = "  ";
    std::ofstream file(persist_path_);
    if (file.is_open()) file << Json::writeString(w, root) << '\n';
}

void FinOpsPlugin::flush_loop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(flush_interval_seconds_));
        if (!running_) break;
        if (dirty_.exchange(false)) {
            std::shared_lock lock(mtx_);
            save_unlocked();
        }
    }
}
