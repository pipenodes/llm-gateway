#include "ollama_client.h"
#include <sstream>
#include <random>

OllamaClient::~OllamaClient() {
    running = false;
    if (health_thread.joinable()) health_thread.join();
    cleanupStreamThreads(true);
}

void OllamaClient::init(const std::vector<std::pair<std::string, int>>& endpoints) {
    for (auto& [h, p] : endpoints) {
        backends.push_back({h, p});
        backend_healthy.push_back(true);
    }
    if (backends.size() > 1) startHealthCheck();
}

OllamaClient::Backend OllamaClient::selectBackend() const {
    if (backends.empty()) return {"localhost", 11434};
    auto count = backends.size();
    std::lock_guard lock(health_mtx);
    for (size_t i = 0; i < count; ++i) {
        auto idx = rr_counter.fetch_add(1) % count;
        if (backend_healthy[idx]) return backends[idx];
    }
    return backends[rr_counter.fetch_add(1) % count];
}

httplib::Client& OllamaClient::getOrCreateClient(const Backend& backend) {
    thread_local ClientPool pool;
    auto key = backend.host + ":" + std::to_string(backend.port);
    auto it = pool.clients.find(key);
    if (it != pool.clients.end())
        return *it->second;
    auto cli = std::make_unique<httplib::Client>(backend.host, backend.port);
    cli->set_connection_timeout(CONN_TIMEOUT, 0);
    cli->set_keep_alive(true);
    auto* ptr = cli.get();
    pool.clients[key] = std::move(cli);
    return *ptr;
}

httplib::Result OllamaClient::request(const std::string& method, const std::string& path,
                                       const std::string& body, int read_timeout) {
    httplib::Result last_result;
    for (int attempt = 0; attempt <= MAX_RETRIES; ++attempt) {
        if (attempt > 0) {
            auto delay = BASE_RETRY_MS * (1 << (attempt - 1));
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            Json::Value f;
            f["attempt"] = attempt;
            f["max"] = MAX_RETRIES;
            Logger::warn("ollama_retry", f);
        }
        auto backend = selectBackend();
        auto& cli = getOrCreateClient(backend);
        cli.set_read_timeout(read_timeout, 0);

        if (method == "POST")
            last_result = cli.Post(path, body, "application/json");
        else
            last_result = cli.Get(path);

        if (last_result && last_result->status == 200) return last_result;
        if (last_result && last_result->status < 500) return last_result;
    }
    return last_result;
}

httplib::Result OllamaClient::showModel(const std::string& model_name) {
    thread_local Json::StreamWriterBuilder w = [] {
        Json::StreamWriterBuilder b;
        b["indentation"] = "";
        return b;
    }();
    Json::Value body;
    body["model"] = model_name;
    return request("POST", "/api/show", Json::writeString(w, body), SHORT_TIMEOUT);
}

void OllamaClient::chatCompletionStream(const std::string& body_str,
                                          const std::string& chat_id,
                                          Json::Int64 created,
                                          const std::string& model,
                                          std::shared_ptr<ChunkQueue> queue) {
    auto backend = selectBackend();
    auto done_flag = std::make_shared<std::atomic<bool>>(false);

    auto t = std::jthread([this, queue, backend, body_str, chat_id,
                            created, model, done_flag] {
        httplib::Client cli(backend.host, backend.port);
        cli.set_connection_timeout(CONN_TIMEOUT, 0);
        cli.set_read_timeout(CHAT_TIMEOUT, 0);

        std::string buffer;
        bool first_chunk = true;
        bool has_tool_calls = false;
        size_t buf_offset = 0;

        Json::CharReaderBuilder reader;
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";

        auto receiver = [&](const char* data, size_t len) -> bool {
            if (queue->cancelled.load(std::memory_order_acquire)) return false;
            buffer.append(data, len);
            size_t pos;
            while ((pos = buffer.find('\n', buf_offset)) != std::string::npos) {
                std::string_view line_view(buffer.data() + buf_offset, pos - buf_offset);
                buf_offset = pos + 1;
                if (line_view.empty()) continue;

                Json::Value chunk;
                std::string errs;
                auto rdr = reader.newCharReader();
                if (!rdr->parse(line_view.data(), line_view.data() + line_view.size(),
                                &chunk, &errs)) continue;

                bool done = chunk.get("done", false).asBool();

                if (done) {
                    if (chunk.isMember("message") &&
                        chunk["message"].isMember("tool_calls") &&
                        chunk["message"]["tool_calls"].isArray() &&
                        chunk["message"]["tool_calls"].size() > 0) {
                        has_tool_calls = true;
                        Json::Value tc_sse;
                        tc_sse["id"] = chat_id;
                        tc_sse["object"] = "chat.completion.chunk";
                        tc_sse["created"] = created;
                        tc_sse["model"] = model;
                        Json::Value tc_choice;
                        tc_choice["index"] = 0;
                        Json::Value tc_delta;
                        tc_delta["tool_calls"] = formatToolCallsStatic(
                            chunk["message"]["tool_calls"]);
                        tc_choice["delta"] = tc_delta;
                        tc_choice["finish_reason"] = Json::Value();
                        Json::Value tc_choices(Json::arrayValue);
                        tc_choices.append(tc_choice);
                        tc_sse["choices"] = tc_choices;
                        queue->push("data: " + Json::writeString(writer, tc_sse) + "\n\n");
                    }

                    Json::Value sse;
                    sse["id"] = chat_id;
                    sse["object"] = "chat.completion.chunk";
                    sse["created"] = created;
                    sse["model"] = model;
                    sse["system_fingerprint"] = "fp_ollama";

                    Json::Value choice;
                    choice["index"] = 0;
                    choice["delta"] = Json::Value(Json::objectValue);

                    std::string done_reason = chunk.get("done_reason", "stop").asString();
                    if (has_tool_calls)
                        choice["finish_reason"] = "tool_calls";
                    else if (done_reason == "length")
                        choice["finish_reason"] = "length";
                    else
                        choice["finish_reason"] = "stop";

                    int pt = chunk.get("prompt_eval_count", 0).asInt();
                    int ct = chunk.get("eval_count", 0).asInt();
                    sse["usage"]["prompt_tokens"] = pt;
                    sse["usage"]["completion_tokens"] = ct;
                    sse["usage"]["total_tokens"] = pt + ct;

                    Json::Value choices(Json::arrayValue);
                    choices.append(choice);
                    sse["choices"] = choices;
                    queue->push("data: " + Json::writeString(writer, sse) + "\n\n");
                } else {
                    if (chunk.isMember("message") &&
                        chunk["message"].isMember("tool_calls") &&
                        chunk["message"]["tool_calls"].isArray() &&
                        chunk["message"]["tool_calls"].size() > 0) {
                        has_tool_calls = true;
                        Json::Value tc_sse;
                        tc_sse["id"] = chat_id;
                        tc_sse["object"] = "chat.completion.chunk";
                        tc_sse["created"] = created;
                        tc_sse["model"] = model;
                        Json::Value tc_choice;
                        tc_choice["index"] = 0;
                        Json::Value tc_delta;
                        if (first_chunk) {
                            tc_delta["role"] = "assistant";
                            first_chunk = false;
                        }
                        tc_delta["tool_calls"] = formatToolCallsStatic(
                            chunk["message"]["tool_calls"]);
                        tc_choice["delta"] = tc_delta;
                        tc_choice["finish_reason"] = Json::Value();
                        Json::Value tc_choices(Json::arrayValue);
                        tc_choices.append(tc_choice);
                        tc_sse["choices"] = tc_choices;
                        queue->push("data: " + Json::writeString(writer, tc_sse) + "\n\n");
                        continue;
                    }

                    Json::Value sse;
                    sse["id"] = chat_id;
                    sse["object"] = "chat.completion.chunk";
                    sse["created"] = created;
                    sse["model"] = model;

                    Json::Value choice;
                    choice["index"] = 0;
                    Json::Value delta;
                    if (first_chunk) {
                        delta["role"] = "assistant";
                        first_chunk = false;
                    }
                    if (chunk.isMember("message") && chunk["message"].isMember("content"))
                        delta["content"] = chunk["message"]["content"];
                    choice["delta"] = delta;
                    choice["finish_reason"] = Json::Value();

                    Json::Value choices(Json::arrayValue);
                    choices.append(choice);
                    sse["choices"] = choices;
                    queue->push("data: " + Json::writeString(writer, sse) + "\n\n");
                }
            }
            if (buf_offset > 0) {
                buffer.erase(0, buf_offset);
                buf_offset = 0;
            }
            return true;
        };

        auto result = cli.Post("/api/chat", httplib::Headers{},
            body_str, "application/json", receiver, nullptr);

        if (!result) {
            Json::Value err;
            err["error"]["message"] = "Cannot connect to Ollama";
            err["error"]["type"] = "server_error";
            queue->push("data: " + Json::writeString(writer, err) + "\n\n");
        }
        queue->push("data: [DONE]\n\n");
        queue->finish();
        done_flag->store(true, std::memory_order_release);
    });
    addStreamThread(std::move(t), done_flag);
}

void OllamaClient::textCompletionStream(const std::string& body_str,
                                          const std::string& completion_id,
                                          Json::Int64 created,
                                          const std::string& model,
                                          bool echo,
                                          const std::string& prompt,
                                          std::shared_ptr<ChunkQueue> queue) {
    auto backend = selectBackend();
    auto done_flag = std::make_shared<std::atomic<bool>>(false);

    auto t = std::jthread([this, queue, backend, body_str, completion_id,
                 created, model, echo, prompt, done_flag] {
        httplib::Client cli(backend.host, backend.port);
        cli.set_connection_timeout(CONN_TIMEOUT, 0);
        cli.set_read_timeout(CHAT_TIMEOUT, 0);

        std::string buffer;
        bool first_chunk = true;
        size_t buf_offset = 0;

        Json::CharReaderBuilder reader;
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";

        auto receiver = [&](const char* data, size_t len) -> bool {
            if (queue->cancelled.load(std::memory_order_acquire)) return false;
            buffer.append(data, len);
            size_t pos;
            while ((pos = buffer.find('\n', buf_offset)) != std::string::npos) {
                std::string_view line_view(buffer.data() + buf_offset, pos - buf_offset);
                buf_offset = pos + 1;
                if (line_view.empty()) continue;

                Json::Value chunk;
                std::string errs;
                auto rdr = reader.newCharReader();
                if (!rdr->parse(line_view.data(), line_view.data() + line_view.size(),
                                &chunk, &errs)) continue;

                bool done = chunk.get("done", false).asBool();

                Json::Value sse;
                sse["id"] = completion_id;
                sse["object"] = "text_completion";
                sse["created"] = created;
                sse["model"] = model;
                sse["system_fingerprint"] = "fp_ollama";

                Json::Value choice;
                choice["index"] = 0;
                choice["logprobs"] = Json::Value();

                if (done) {
                    std::string done_reason = chunk.get("done_reason", "stop").asString();
                    choice["text"] = "";
                    choice["finish_reason"] = (done_reason == "length") ? "length" : "stop";

                    int pt = chunk.get("prompt_eval_count", 0).asInt();
                    int ct = chunk.get("eval_count", 0).asInt();
                    sse["usage"]["prompt_tokens"] = pt;
                    sse["usage"]["completion_tokens"] = ct;
                    sse["usage"]["total_tokens"] = pt + ct;
                } else {
                    std::string text = chunk.get("response", "").asString();
                    if (first_chunk && echo) {
                        text = prompt + text;
                    }
                    first_chunk = false;
                    choice["text"] = text;
                    choice["finish_reason"] = Json::Value();
                }

                Json::Value choices(Json::arrayValue);
                choices.append(choice);
                sse["choices"] = choices;
                queue->push("data: " + Json::writeString(writer, sse) + "\n\n");
            }
            if (buf_offset > 0) {
                buffer.erase(0, buf_offset);
                buf_offset = 0;
            }
            return true;
        };

        auto result = cli.Post("/api/generate", httplib::Headers{},
            body_str, "application/json", receiver, nullptr);

        if (!result) {
            Json::Value err;
            err["error"]["message"] = "Cannot connect to Ollama";
            err["error"]["type"] = "server_error";
            queue->push("data: " + Json::writeString(writer, err) + "\n\n");
        }
        queue->push("data: [DONE]\n\n");
        queue->finish();
        done_flag->store(true, std::memory_order_release);
    });
    addStreamThread(std::move(t), done_flag);
}

Json::Value OllamaClient::formatToolCallsStatic(const Json::Value& ollama_tool_calls) {
    Json::StreamWriterBuilder tw;
    tw["indentation"] = "";

    Json::Value result(Json::arrayValue);
    int idx = 0;
    for (const auto& tc : ollama_tool_calls) {
        Json::Value call;
        auto call_id = "call_" + crypto::secure_random(ID_RANDOM_LENGTH, ID_CHARSET);
        call["id"] = call_id;
        call["index"] = idx++;
        call["type"] = "function";
        call["function"]["name"] = tc["function"]["name"];
        if (tc["function"]["arguments"].isString())
            call["function"]["arguments"] = tc["function"]["arguments"];
        else
            call["function"]["arguments"] = Json::writeString(tw, tc["function"]["arguments"]);
        result.append(call);
    }
    return result;
}

void OllamaClient::addStreamThread(std::jthread&& t, std::shared_ptr<std::atomic<bool>> done) {
    std::lock_guard lock(stream_mtx);
    std::erase_if(stream_threads, [](const StreamThread& st) {
        return st.done->load(std::memory_order_acquire);
    });
    stream_threads.push_back({std::move(t), std::move(done)});
}

void OllamaClient::cleanupStreamThreads(bool join_all) {
    std::lock_guard lock(stream_mtx);
    if (join_all) {
        stream_threads.clear();
    } else {
        std::erase_if(stream_threads, [](const StreamThread& st) {
            return st.done->load(std::memory_order_acquire);
        });
    }
}

void OllamaClient::startHealthCheck() {
    health_thread = std::thread([this] {
        while (running) {
            for (size_t i = 0; i < backends.size(); ++i) {
                httplib::Client cli(backends[i].host, backends[i].port);
                cli.set_connection_timeout(3, 0);
                cli.set_read_timeout(3, 0);
                auto res = cli.Get("/api/tags");
                std::lock_guard lock(health_mtx);
                backend_healthy[i] = (res && res->status == 200);
            }
            for (int s = 0; s < HEALTH_INTERVAL_SECS && running; ++s)
                std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
}
