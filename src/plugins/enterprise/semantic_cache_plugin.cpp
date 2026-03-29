#include "semantic_cache_plugin.h"
#include "../logger.h"
#include "../crypto.h"
#include <algorithm>
#include <cmath>
#include <sstream>

bool SemanticCachePlugin::init(const Json::Value& config) {
    if (!config.isObject()) return true;
    threshold_ = config.get("threshold", 0.95).asFloat();
    max_entries_ = static_cast<size_t>(config.get("max_entries", 5000).asInt());
    ttl_seconds_ = config.get("ttl_seconds", 3600).asInt();
    embedding_model_ = config.get("embedding_model", "nomic-embed-text").asString();
    only_temperature_zero_ = config.get("only_temperature_zero", true).asBool();
    excluded_models_.clear();
    if (config.isMember("excluded_models") && config["excluded_models"].isArray()) {
        for (const auto& m : config["excluded_models"])
            excluded_models_.push_back(m.asString());
    }
    if (config.isMember("ollama_backends") && config["ollama_backends"].isArray()) {
        ollama_backends_.clear();
        for (const auto& b : config["ollama_backends"]) {
            if (b.isArray() && b.size() >= 2)
                ollama_backends_.emplace_back(b[0].asString(), b[1].asInt());
            else if (b.isString()) {
                std::string s = b.asString();
                size_t colon = s.find(':');
                int port = 11434;
                if (colon != std::string::npos) {
                    try { port = std::stoi(s.substr(colon + 1)); } catch (...) {}
                    ollama_backends_.emplace_back(s.substr(0, colon), port);
                } else {
                    ollama_backends_.emplace_back(s, 11434);
                }
            }
        }
    }
    if (ollama_backends_.empty())
        ollama_backends_.emplace_back("localhost", 11434);
    if (!ollama_) {
        own_ollama_ = std::make_unique<OllamaClient>();
        own_ollama_->init(ollama_backends_);
    }
    return true;
}

void SemanticCachePlugin::shutdown() {
    own_ollama_.reset();
    ollama_ = nullptr;
}

OllamaClient* SemanticCachePlugin::get_ollama() {
    if (ollama_) return ollama_;
    return own_ollama_.get();
}

std::string SemanticCachePlugin::messages_to_prompt(const Json::Value& messages) {
    if (!messages.isArray()) return "";
    std::ostringstream out;
    for (const auto& m : messages) {
        if (m.isObject() && m.isMember("content")) {
            const auto& c = m["content"];
            if (c.isString())
                out << c.asString() << "\n";
            else if (c.isArray()) {
                for (const auto& part : c) {
                    if (part.isObject() && part.isMember("text"))
                        out << part["text"].asString() << "\n";
                }
            }
        }
    }
    return out.str();
}

std::vector<float> SemanticCachePlugin::get_embedding(
    const std::string& text, const std::string&) const {
    auto* client = const_cast<SemanticCachePlugin*>(this)->get_ollama();
    if (!client) return {};
    thread_local Json::StreamWriterBuilder w = [] {
        Json::StreamWriterBuilder b;
        b["indentation"] = "";
        return b;
    }();
    Json::Value body;
    body["model"] = embedding_model_;
    body["input"] = text;
    auto res = client->embeddings(Json::writeString(w, body));
    if (!res || res->status != 200) return {};
    std::string errs;
    Json::Value j;
    Json::CharReaderBuilder r;
    if (!r.newCharReader()->parse(res->body.data(),
            res->body.data() + res->body.size(), &j, &errs))
        return {};
    if (!j.isMember("embeddings") || !j["embeddings"].isArray()
        || j["embeddings"].empty())
        return {};
    std::vector<float> vec;
    for (const auto& v : j["embeddings"][0])
        vec.push_back(static_cast<float>(v.asDouble()));
    return vec;
}

float SemanticCachePlugin::cosine_similarity(
    const std::vector<float>& a, const std::vector<float>& b) const noexcept {
    if (a.size() != b.size() || a.empty()) return 0.f;
    double dot = 0, na = 0, nb = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += static_cast<double>(a[i]) * b[i];
        na += static_cast<double>(a[i]) * a[i];
        nb += static_cast<double>(b[i]) * b[i];
    }
    double denom = std::sqrt(na) * std::sqrt(nb);
    return denom > 0 ? static_cast<float>(dot / denom) : 0.f;
}

void SemanticCachePlugin::evict_expired() {
    int64_t now = static_cast<int64_t>(crypto::epoch_seconds());
    std::unique_lock lock(mtx_);
    entries_.erase(
        std::remove_if(entries_.begin(), entries_.end(),
            [this, now](const CacheEntry& e) {
                return (now - e.created_at) > ttl_seconds_;
            }),
        entries_.end());
}

void SemanticCachePlugin::evict_lru() {
    if (entries_.size() < max_entries_) return;
    std::unique_lock lock(mtx_);
    if (entries_.size() < max_entries_) return;
    std::sort(entries_.begin(), entries_.end(),
        [](const CacheEntry& a, const CacheEntry& b) {
            return a.last_hit < b.last_hit;
        });
    entries_.erase(entries_.begin(),
        entries_.begin() + static_cast<ptrdiff_t>(entries_.size() - max_entries_ / 2));
}

PluginResult SemanticCachePlugin::before_request(
    Json::Value& body, PluginRequestContext& ctx) {
    if (ctx.path.find("/embeddings") != std::string::npos)
        return PluginResult{.action = PluginAction::Continue};
    std::string model = body.get("model", "").asString();
    for (const auto& ex : excluded_models_)
        if (model == ex) return PluginResult{.action = PluginAction::Continue};
    if (only_temperature_zero_) {
        double temp = body.get("temperature", -1.0).asDouble();
        if (temp >= 0 && temp != 0.0) return PluginResult{.action = PluginAction::Continue};
    }
    if (!body.isMember("messages") || !body["messages"].isArray())
        return PluginResult{.action = PluginAction::Continue};

    std::string prompt = messages_to_prompt(body["messages"]);
    if (prompt.empty()) return PluginResult{.action = PluginAction::Continue};

    std::vector<float> emb = get_embedding(prompt, model);
    if (emb.empty()) return PluginResult{.action = PluginAction::Continue};

    evict_expired();

    std::shared_lock lock(mtx_);
    float best_sim = 0.f;
    const CacheEntry* best = nullptr;
    for (const auto& e : entries_) {
        if (e.model != model) continue;
        float sim = cosine_similarity(emb, e.embedding);
        if (sim >= threshold_ && sim > best_sim) {
            best_sim = sim;
            best = &e;
        }
    }

    if (best) {
        hits_++;
        std::unique_lock ulock(mtx_);
        for (auto& e : entries_) {
            if (&e == best) {
                e.last_hit = static_cast<int64_t>(crypto::epoch_seconds());
                e.hit_count++;
                break;
            }
        }
        PluginResult r{.action = PluginAction::Block};
        r.cached_response = best->response_json;
        r.response_headers["X-Cache"] = "SEMANTIC-HIT";
        r.response_headers["X-Semantic-Similarity"] = std::to_string(best_sim);
        return r;
    }

    misses_++;
    ctx.metadata["X-Cache"] = "SEMANTIC-MISS";
    ctx.metadata["_semantic_prompt"] = prompt;
    return PluginResult{.action = PluginAction::Continue};
}

PluginResult SemanticCachePlugin::after_response(
    Json::Value& response, PluginRequestContext& ctx) {
    if (ctx.path.find("/embeddings") != std::string::npos)
        return PluginResult{.action = PluginAction::Continue};
    if (ctx.metadata.count("X-Cache") && ctx.metadata.at("X-Cache") == "SEMANTIC-HIT")
        return PluginResult{.action = PluginAction::Continue};

    std::string model = ctx.model;
    for (const auto& ex : excluded_models_)
        if (model == ex) return PluginResult{.action = PluginAction::Continue};

    std::string prompt;
    if (ctx.metadata.count("_semantic_prompt"))
        prompt = ctx.metadata.at("_semantic_prompt");
    if (prompt.empty()) return PluginResult{.action = PluginAction::Continue};

    std::vector<float> emb = get_embedding(prompt, model);
    if (emb.empty()) return PluginResult{.action = PluginAction::Continue};

    thread_local Json::StreamWriterBuilder w = [] {
        Json::StreamWriterBuilder b;
        b["indentation"] = "";
        return b;
    }();
    std::string response_json = Json::writeString(w, response);

    evict_expired();
    evict_lru();

    int64_t now = static_cast<int64_t>(crypto::epoch_seconds());
    CacheEntry e;
    e.embedding = std::move(emb);
    e.response_json = std::move(response_json);
    e.model = model;
    e.created_at = now;
    e.last_hit = now;
    e.hit_count = 0;

    std::unique_lock lock(mtx_);
    if (entries_.size() >= max_entries_) evict_lru();
    entries_.push_back(std::move(e));
    return PluginResult{.action = PluginAction::Continue};
}

Json::Value SemanticCachePlugin::stats() const {
    Json::Value s;
    s["hits"] = static_cast<Json::Int64>(hits_.load());
    s["misses"] = static_cast<Json::Int64>(misses_.load());
    std::shared_lock lock(mtx_);
    s["entries"] = static_cast<Json::Int64>(entries_.size());
    return s;
}
