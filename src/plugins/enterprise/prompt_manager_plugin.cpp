#include "prompt_manager_plugin.h"
#include "logger.h"
#include "crypto.h"
#include <fstream>
#include <sstream>

bool PromptManagerPlugin::init(const Json::Value& config) {
    persist_path_ = config.get("persist_path", "prompts.json").asString();
    default_system_prompt_ = config.get("default_system_prompt", "").asString();
    system_prompt_mode_ = config.get("system_prompt_mode", "prepend").asString();
    guardrails_enabled_ = config.get("guardrails_enabled", true).asBool();

    if (config.isMember("key_prompts") && config["key_prompts"].isObject()) {
        for (const auto& k : config["key_prompts"].getMemberNames())
            key_prompts_[k] = config["key_prompts"][k].asString();
    }

    if (config.isMember("templates") && config["templates"].isObject()) {
        for (const auto& tname : config["templates"].getMemberNames()) {
            const auto& t = config["templates"][tname];
            PromptTemplate tmpl;
            tmpl.name = tname;
            tmpl.content = t.get("content", "").asString();
            if (t.isMember("variables") && t["variables"].isArray()) {
                for (const auto& v : t["variables"])
                    tmpl.variables.push_back(v.asString());
            }
            tmpl.created_at = crypto::epoch_seconds();
            templates_[tname] = std::move(tmpl);
        }
    }

    if (config.isMember("guardrails") && config["guardrails"].isArray()) {
        for (const auto& g : config["guardrails"]) {
            GuardrailRule rule;
            rule.name = g.get("name", "").asString();
            rule.pattern = g.get("pattern", "").asString();
            rule.action = g.get("action", "block").asString();
            rule.message = g.get("message", "Content blocked").asString();
            try {
                rule.compiled = std::regex(rule.pattern, std::regex::icase | std::regex::optimize);
                guardrails_.push_back(std::move(rule));
            } catch (const std::regex_error& e) {
                Json::Value f;
                f["rule"] = rule.name;
                f["error"] = e.what();
                Logger::warn("guardrail_regex_error", f);
            }
        }
    }

    load();

    Json::Value f;
    f["templates"] = static_cast<int>(templates_.size());
    f["guardrails"] = static_cast<int>(guardrails_.size());
    f["key_prompts"] = static_cast<int>(key_prompts_.size());
    Logger::info("prompt_manager_init", f);
    return true;
}

void PromptManagerPlugin::shutdown() {
    std::shared_lock lock(mtx_);
    save();
}

PluginResult PromptManagerPlugin::before_request(Json::Value& body, PluginRequestContext& ctx) {
    std::shared_lock lock(mtx_);

    inject_system_prompt(body, ctx.key_alias);

    if (ctx.metadata.count("prompt_template")) {
        Json::Value vars(Json::objectValue);
        if (ctx.metadata.count("template_vars")) {
            Json::CharReaderBuilder builder;
            std::string errs;
            auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
            const auto& raw = ctx.metadata["template_vars"];
            reader->parse(raw.data(), raw.data() + raw.size(), &vars, &errs);
        }
        apply_template(body, ctx.metadata["prompt_template"], vars);
    }

    if (guardrails_enabled_ && body.isMember("messages")) {
        apply_redactions(body["messages"]);

        auto result = check_guardrails(body["messages"]);
        if (!result.allowed && result.action == "block") {
            return PluginResult{
                .action = PluginAction::Block,
                .error_code = 400,
                .error_message = result.reason
            };
        }
        if (!result.allowed && result.action == "warn") {
            Json::Value f;
            f["rule"] = result.rule_name;
            f["reason"] = result.reason;
            f["key_alias"] = ctx.key_alias;
            Logger::warn("guardrail_warning", f);
        }
    }

    return PluginResult{.action = PluginAction::Continue};
}

PluginResult PromptManagerPlugin::after_response(Json::Value&, PluginRequestContext&) {
    return PluginResult{.action = PluginAction::Continue};
}

Json::Value PromptManagerPlugin::stats() const {
    std::shared_lock lock(mtx_);
    Json::Value out;
    out["templates"] = static_cast<int>(templates_.size());
    out["guardrails"] = static_cast<int>(guardrails_.size());
    out["key_prompts"] = static_cast<int>(key_prompts_.size());
    out["guardrails_enabled"] = guardrails_enabled_;
    return out;
}

void PromptManagerPlugin::inject_system_prompt(Json::Value& body, const std::string& key_alias) const {
    std::string prompt;
    auto it = key_prompts_.find(key_alias);
    if (it != key_prompts_.end()) {
        prompt = it->second;
    } else if (!default_system_prompt_.empty()) {
        prompt = default_system_prompt_;
    }

    if (prompt.empty() || !body.isMember("messages") || !body["messages"].isArray())
        return;

    auto& messages = body["messages"];

    bool has_system = false;
    for (Json::ArrayIndex i = 0; i < messages.size(); ++i) {
        if (messages[i].get("role", "").asString() == "system") {
            has_system = true;
            if (system_prompt_mode_ == "replace") {
                messages[i]["content"] = prompt;
            } else if (system_prompt_mode_ == "prepend") {
                messages[i]["content"] = prompt + "\n\n" + messages[i]["content"].asString();
            } else if (system_prompt_mode_ == "append") {
                messages[i]["content"] = messages[i]["content"].asString() + "\n\n" + prompt;
            }
            break;
        }
    }

    if (!has_system) {
        Json::Value sys_msg;
        sys_msg["role"] = "system";
        sys_msg["content"] = prompt;
        Json::Value new_messages(Json::arrayValue);
        new_messages.append(sys_msg);
        for (Json::ArrayIndex i = 0; i < messages.size(); ++i)
            new_messages.append(messages[i]);
        body["messages"] = std::move(new_messages);
    }
}

bool PromptManagerPlugin::apply_template(Json::Value& body, const std::string& template_name,
                                          const Json::Value& variables) const {
    auto it = templates_.find(template_name);
    if (it == templates_.end()) return false;

    std::string resolved = resolve_template(it->second.content, variables);

    if (body.isMember("messages") && body["messages"].isArray()) {
        auto& messages = body["messages"];
        if (messages.size() > 0) {
            auto& last = messages[messages.size() - 1];
            if (last.get("role", "").asString() == "user") {
                last["content"] = resolved;
                return true;
            }
        }
        Json::Value msg;
        msg["role"] = "user";
        msg["content"] = resolved;
        messages.append(msg);
    }
    return true;
}

PromptManagerPlugin::GuardrailResult
PromptManagerPlugin::check_guardrails(const Json::Value& messages) const {
    for (Json::ArrayIndex i = 0; i < messages.size(); ++i) {
        const auto& msg = messages[i];
        if (msg.get("role", "").asString() == "system") continue;

        std::string content = msg.get("content", "").asString();
        if (content.empty()) continue;

        for (const auto& rule : guardrails_) {
            if (std::regex_search(content, rule.compiled)) {
                return GuardrailResult{
                    .allowed = false,
                    .reason = rule.message,
                    .rule_name = rule.name,
                    .action = rule.action
                };
            }
        }
    }
    return GuardrailResult{.allowed = true};
}

std::string PromptManagerPlugin::resolve_template(const std::string& content,
                                                    const Json::Value& vars) {
    std::string result = content;
    if (!vars.isObject()) return result;

    for (const auto& key : vars.getMemberNames()) {
        std::string placeholder = "{{" + key + "}}";
        std::string value = vars[key].asString();
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.size(), value);
            pos += value.size();
        }
    }
    return result;
}

void PromptManagerPlugin::apply_redactions(Json::Value& messages) const {
    for (Json::ArrayIndex i = 0; i < messages.size(); ++i) {
        if (messages[i].get("role", "").asString() == "system") continue;
        std::string content = messages[i].get("content", "").asString();
        if (content.empty()) continue;

        bool modified = false;
        for (const auto& rule : guardrails_) {
            if (rule.action != "redact") continue;
            std::string replaced = std::regex_replace(content, rule.compiled, "[REDACTED]");
            if (replaced != content) {
                content = std::move(replaced);
                modified = true;
                Json::Value f;
                f["rule"] = rule.name;
                f["message_index"] = static_cast<int>(i);
                Logger::info("guardrail_redacted", f);
            }
        }
        if (modified)
            messages[i]["content"] = content;
    }
}

void PromptManagerPlugin::set_key_system_prompt(const std::string& alias, const std::string& prompt) {
    std::unique_lock lock(mtx_);
    key_prompts_[alias] = prompt;
    save();
}

bool PromptManagerPlugin::add_template(const std::string& tname, const std::string& content,
                                        const std::vector<std::string>& variables) {
    std::unique_lock lock(mtx_);
    PromptTemplate tmpl;
    tmpl.name = tname;
    tmpl.content = content;
    tmpl.variables = variables;
    tmpl.created_at = crypto::epoch_seconds();
    tmpl.version = 1;
    auto existing = templates_.find(tname);
    if (existing != templates_.end())
        tmpl.version = existing->second.version + 1;
    templates_[tname] = std::move(tmpl);
    save();
    return true;
}

Json::Value PromptManagerPlugin::list_templates() const {
    std::shared_lock lock(mtx_);
    Json::Value out(Json::arrayValue);
    for (const auto& [tname, tmpl] : templates_) {
        Json::Value t;
        t["name"] = tmpl.name;
        t["content"] = tmpl.content;
        Json::Value vars(Json::arrayValue);
        for (const auto& v : tmpl.variables) vars.append(v);
        t["variables"] = vars;
        t["created_at"] = static_cast<Json::Int64>(tmpl.created_at);
        t["version"] = tmpl.version;
        out.append(t);
    }
    return out;
}

Json::Value PromptManagerPlugin::list_guardrails() const {
    std::shared_lock lock(mtx_);
    Json::Value out(Json::arrayValue);
    for (const auto& rule : guardrails_) {
        Json::Value g;
        g["name"] = rule.name;
        g["pattern"] = rule.pattern;
        g["action"] = rule.action;
        g["message"] = rule.message;
        out.append(g);
    }
    return out;
}

void PromptManagerPlugin::load() {
    if (persist_path_.empty()) return;
    std::ifstream file(persist_path_);
    if (!file.is_open()) return;

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    if (!Json::parseFromStream(builder, file, &root, &errs) || !root.isObject()) return;

    std::unique_lock lock(mtx_);

    if (root.isMember("key_prompts") && root["key_prompts"].isObject()) {
        for (const auto& k : root["key_prompts"].getMemberNames())
            key_prompts_[k] = root["key_prompts"][k].asString();
    }

    if (root.isMember("templates") && root["templates"].isObject()) {
        for (const auto& tname : root["templates"].getMemberNames()) {
            const auto& t = root["templates"][tname];
            PromptTemplate tmpl;
            tmpl.name = tname;
            tmpl.content = t.get("content", "").asString();
            tmpl.created_at = t.get("created_at", 0).asInt64();
            tmpl.version = t.get("version", 1).asInt();
            if (t.isMember("variables") && t["variables"].isArray()) {
                for (const auto& v : t["variables"])
                    tmpl.variables.push_back(v.asString());
            }
            templates_[tname] = std::move(tmpl);
        }
    }

    if (root.isMember("guardrails") && root["guardrails"].isArray()) {
        for (const auto& g : root["guardrails"]) {
            GuardrailRule rule;
            rule.name = g.get("name", "").asString();
            rule.pattern = g.get("pattern", "").asString();
            rule.action = g.get("action", "block").asString();
            rule.message = g.get("message", "Content blocked").asString();
            try {
                rule.compiled = std::regex(rule.pattern, std::regex::icase | std::regex::optimize);
                guardrails_.push_back(std::move(rule));
            } catch (const std::regex_error&) {}
        }
    }

    Json::Value f;
    f["key_prompts"] = static_cast<int>(key_prompts_.size());
    f["templates"] = static_cast<int>(templates_.size());
    f["guardrails"] = static_cast<int>(guardrails_.size());
    Logger::info("prompt_data_loaded", f);
}

void PromptManagerPlugin::save() const {
    if (persist_path_.empty()) return;

    Json::StreamWriterBuilder w;
    w["indentation"] = "  ";
    Json::Value root(Json::objectValue);

    Json::Value kp(Json::objectValue);
    for (const auto& [alias, prompt] : key_prompts_)
        kp[alias] = prompt;
    root["key_prompts"] = kp;

    Json::Value tmpls(Json::objectValue);
    for (const auto& [tname, tmpl] : templates_) {
        Json::Value t;
        t["content"] = tmpl.content;
        t["created_at"] = static_cast<Json::Int64>(tmpl.created_at);
        t["version"] = tmpl.version;
        Json::Value vars(Json::arrayValue);
        for (const auto& v : tmpl.variables) vars.append(v);
        t["variables"] = vars;
        tmpls[tname] = t;
    }
    root["templates"] = tmpls;

    Json::Value glist(Json::arrayValue);
    for (const auto& rule : guardrails_) {
        Json::Value g;
        g["name"] = rule.name;
        g["pattern"] = rule.pattern;
        g["action"] = rule.action;
        g["message"] = rule.message;
        glist.append(g);
    }
    root["guardrails"] = glist;

    std::ofstream file(persist_path_);
    if (file.is_open())
        file << Json::writeString(w, root) << '\n';
}
