FROM debian:12-slim AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    libjsoncpp-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    libyaml-cpp-dev \
    wget \
    && rm -rf /var/lib/apt/lists/*

COPY . /build
WORKDIR /build

RUN wget -q https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.34.0/httplib.h -O src/httplib.h

RUN g++ -o hermes \
    src/main.cpp src/gateway.cpp src/plugin.cpp src/update_checker.cpp src/core_services.cpp \
    src/request_queue.cpp src/benchmark.cpp \
    src/plugins/logging_plugin.cpp src/plugins/semantic_cache_plugin.cpp \
    src/plugins/cache_plugin.cpp src/plugins/auth_plugin.cpp src/plugins/rate_limit_plugin.cpp src/plugins/request_queue_plugin.cpp \
    src/plugins/pii_redaction_plugin.cpp src/plugins/content_moderation_plugin.cpp src/plugins/prompt_injection_plugin.cpp \
    src/plugins/response_validator_plugin.cpp src/plugins/rag_injector_plugin.cpp src/plugins/cost_controller_plugin.cpp \
    src/plugins/request_router_plugin.cpp src/plugins/conversation_memory_plugin.cpp src/plugins/adaptive_rate_limiter_plugin.cpp \
    src/plugins/streaming_transformer_plugin.cpp src/plugins/api_versioning_plugin.cpp \
    src/plugins/request_deduplication_plugin.cpp src/plugins/model_warmup_plugin.cpp \
    src/plugins/usage_tracking_plugin.cpp src/plugins/prompt_manager_plugin.cpp \
    src/plugins/ab_testing_plugin.cpp src/plugins/webhook_plugin.cpp \
    src/plugins/audit_plugin.cpp src/plugins/alerting_plugin.cpp \
    src/plugins/enterprise/guardrails_plugin.cpp \
    src/plugins/enterprise/data_discovery_plugin.cpp \
    src/plugins/enterprise/dlp_plugin.cpp \
    src/plugins/enterprise/finops_plugin.cpp \
    src/ollama_client.cpp src/api_keys.cpp \
    src/ollama_provider.cpp src/openai_custom_provider.cpp src/provider_router.cpp \
    src/discovery/config_validator.cpp src/discovery/configuration_watcher.cpp \
    src/discovery/file_provider.cpp src/discovery/docker_provider.cpp \
    src/discovery/kubernetes_provider.cpp \
    -std=c++23 -O3 -Wall -Wextra -DNDEBUG -DCPPHTTPLIB_OPENSSL_SUPPORT -march=x86-64-v2 -flto=4 -pipe \
    -ffunction-sections -fdata-sections -Wno-deprecated-declarations -Wno-missing-field-initializers \
    -I/usr/include/jsoncpp -Isrc \
    -Wl,--gc-sections -s \
    -ljsoncpp -lpthread -lssl -lcrypto -lcurl -lyaml-cpp

FROM debian:12-slim

RUN apt-get update && apt-get install -y \
    libjsoncpp25 \
    libssl3 \
    libcurl4 \
    libyaml-cpp0.7 \
    curl \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /build/hermes /app/hermes
COPY config.json /app/data/config.json

RUN mkdir -p /app/data

WORKDIR /app/data
EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=5s --retries=3 \
    CMD curl -sf http://localhost:8080/health || exit 1

VOLUME /app/data
CMD ["/app/hermes"]
