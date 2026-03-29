#include "pii_redaction_plugin.h"
#include "../logger.h"
#include <sstream>
#include <algorithm>
#include <vector>

static const std::string PII_METADATA_KEY = "pii_redactor_map";

bool PIIRedactionPlugin::init(const Json::Value& config) {
    log_redactions_ = config.get("log_redactions", true).asBool();
    patterns_.clear();
    auto add = [this](const std::string& name, const std::string& regex_str, const std::string& prefix) {
        try {
            patterns_.push_back({name, std::regex(regex_str), prefix});
        } catch (const std::regex_error&) {}
    };
    if (config.isMember("patterns") && config["patterns"].isArray()) {
        for (const auto& p : config["patterns"]) {
            std::string n = p.asString();
            if (n == "cpf") add("cpf", R"(\b\d{3}\.\d{3}\.\d{3}-\d{2}\b)", "[CPF_");
            else if (n == "email") add("email", R"(\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b)", "[EMAIL_");
            else if (n == "phone") add("phone", R"(\b\(?\d{2}\)?[\s-]?\d{4,5}-?\d{4}\b)", "[PHONE_");
            else if (n == "credit_card") add("credit_card", R"(\b\d{4}[\s-]?\d{4}[\s-]?\d{4}[\s-]?\d{4}\b)", "[CARD_");
            else if (n == "cnpj") add("cnpj", R"(\b\d{2}\.\d{3}\.\d{3}/\d{4}-\d{2}\b)", "[CNPJ_");
        }
    }
    if (patterns_.empty()) {
        add("cpf", R"(\b\d{3}\.\d{3}\.\d{3}-\d{2}\b)", "[CPF_");
        add("email", R"(\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b)", "[EMAIL_");
        add("phone", R"(\b\(?\d{2}\)?[\s-]?\d{4,5}-?\d{4}\b)", "[PHONE_");
    }
    return true;
}

std::string PIIRedactionPlugin::redact_text(const std::string& text,
    std::unordered_map<std::string, std::string>& map) {
    std::string out = text;
    for (auto& pat : patterns_) {
        std::vector<std::pair<size_t, std::string>> matches;
        for (std::sregex_iterator it(out.begin(), out.end(), pat.pattern), end; it != end; ++it) {
            matches.emplace_back(it->position(), it->str());
        }
        int idx = 0;
        for (auto it = matches.rbegin(); it != matches.rend(); ++it) {
            std::string placeholder = pat.placeholder_prefix + std::to_string(++idx) + "]";
            if (map.find(placeholder) == map.end())
                map[placeholder] = it->second;
            out.replace(it->first, it->second.size(), placeholder);
        }
    }
    return out;
}

std::string PIIRedactionPlugin::restore_text(const std::string& text,
    const std::unordered_map<std::string, std::string>& map) const {
    std::string out = text;
    for (const auto& [ph, val] : map) {
        size_t pos = 0;
        while ((pos = out.find(ph, pos)) != std::string::npos) {
            out.replace(pos, ph.size(), val);
            pos += val.size();
        }
    }
    return out;
}

void PIIRedactionPlugin::redact_messages(Json::Value& messages,
    std::unordered_map<std::string, std::string>& map) {
    if (!messages.isArray()) return;
    for (auto& msg : messages) {
        if (msg.isMember("content") && msg["content"].isString()) {
            std::string c = msg["content"].asString();
            msg["content"] = redact_text(c, map);
        }
    }
}

std::string PIIRedactionPlugin::serialize_map(const std::unordered_map<std::string, std::string>& map) {
    std::ostringstream os;
    for (const auto& [k, v] : map) {
        os << k << "\t" << v << "\n";
    }
    return os.str();
}

bool PIIRedactionPlugin::deserialize_map(const std::string& s,
    std::unordered_map<std::string, std::string>& map) {
    map.clear();
    std::istringstream is(s);
    std::string line, key, val;
    while (std::getline(is, line)) {
        auto tab = line.find('\t');
        if (tab != std::string::npos) {
            key = line.substr(0, tab);
            val = line.substr(tab + 1);
            map[key] = val;
        }
    }
    return !map.empty();
}

PluginResult PIIRedactionPlugin::before_request(Json::Value& body, PluginRequestContext& ctx) {
    if (!body.isMember("messages") || !body["messages"].isArray())
        return PluginResult{.action = PluginAction::Continue};
    std::unordered_map<std::string, std::string> map;
    redact_messages(body["messages"], map);
    if (!map.empty()) {
        ctx.metadata[PII_METADATA_KEY] = serialize_map(map);
        if (log_redactions_) {
            Json::Value f;
            f["count"] = static_cast<int>(map.size());
            Logger::info("pii_redacted", f);
        }
    }
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult PIIRedactionPlugin::after_response(Json::Value& response, PluginRequestContext& ctx) {
    auto it = ctx.metadata.find(PII_METADATA_KEY);
    if (it == ctx.metadata.end()) return PluginResult{.action = PluginAction::Continue};
    std::unordered_map<std::string, std::string> map;
    if (!deserialize_map(it->second, map)) return PluginResult{.action = PluginAction::Continue};
    if (response.isMember("choices") && response["choices"].isArray() && response["choices"].size() > 0) {
        auto& choice = response["choices"][0];
        if (choice.isMember("message") && choice["message"].isMember("content")
            && choice["message"]["content"].isString()) {
            std::string c = choice["message"]["content"].asString();
            choice["message"]["content"] = restore_text(c, map);
        }
    }
    return PluginResult{.action = PluginAction::Continue};
}
