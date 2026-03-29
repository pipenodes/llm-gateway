# HERMES — Solution Summary

HERMES (Hybrid Engine for Reasoning, Models & Execution Services) e um gateway HTTP em C++23 que expoe uma API compativel com OpenAI, roteando requisicoes para backends LLM com pipeline de plugins extensivel para seguranca, cache, auditoria e compliance.

## Stack Tecnica

| Aspecto | Tecnologia |
|---------|------------|
| Linguagem | C++23 |
| HTTP Server/Client | cpp-httplib v0.34.0 (header-only) |
| JSON | jsoncpp |
| YAML | yaml-cpp (discovery) |
| TLS | OpenSSL |
| HTTP Client | libcurl (providers externos) |
| Build | Makefile (g++, LTO, -O3, -march=x86-64-v2) |
| Container | Docker (Debian 12-slim, multi-stage) |
| Orquestracao | Kubernetes (Kustomize) |
| CI/CD | GitHub Actions (build, asan, tsan, edge, docker) |

## Arquitetura

### Core minimo + plugins

- **Core:** Gateway HTTP, Config, Logger, OllamaClient, PluginManager, interfaces (ICache, IAuth, IRateLimiter, IRequestQueue, IMetricsCollector, IAuditSink)
- **Plugins builtin:** Implementacoes das interfaces + plugins de pipeline (17 plugins)
- **Build edge:** Core + implementacoes null (NullCache, NullAuth, NullRateLimiter, NullRequestQueue)

### Pipeline de plugins

1. `before_request` — plugins processam na ordem do pipeline
2. Backend LLM — request enviada ao provider resolvido
3. `after_response` — plugins processam na ordem inversa
4. `notify_request_completed` — plugins IAuditSink recebem o evento

### Providers

- **OllamaProvider** — round-robin, health check, retry com backoff
- **OpenAICustomProvider** — proxy para OpenRouter, Azure, custom OpenAI-compativel

## Endpoints

### API OpenAI-Compativel

| Metodo | Path | Descricao |
|--------|------|-----------|
| POST | /v1/chat/completions | Chat completions (streaming e non-streaming, tools, json_object) |
| POST | /v1/completions | Text completions (legacy) |
| POST | /v1/embeddings | Geracao de embeddings |
| GET | /v1/models | Lista modelos + aliases + discovery |
| GET | /v1/models/{id} | Detalhes de um modelo |
| POST | /v1/benchmark | Benchmark ad-hoc (se BENCHMARK_ENABLED) |

### Administracao (requer ADMIN_KEY)

| Metodo | Path | Descricao |
|--------|------|-----------|
| POST | /admin/keys | Criar API key |
| GET | /admin/keys | Listar API keys |
| DELETE | /admin/keys/{alias} | Revogar API key |
| GET | /admin/plugins | Listar plugins |
| GET | /admin/audit | Consultar audit log |
| GET | /admin/posture | Relatorio de postura ASPM |
| GET | /admin/compliance/report | Relatorio de compliance |
| POST | /admin/security-scan | Testes de seguranca proativos |
| GET | /admin/discovery | Status do discovery dinamico |
| POST | /admin/discovery/refresh | Forcar refresh do discovery |
| GET | /admin/config | Configuracao atual |
| GET | /admin/queue | Estado da fila |
| DELETE | /admin/queue | Limpar fila |
| GET | /admin/updates/check | Verificar atualizacoes |
| POST | /admin/updates/apply | Aplicar atualizacao |

### Sistema

| Metodo | Path | Descricao |
|--------|------|-----------|
| GET | /health | Health check |
| GET | /metrics | Metricas JSON |
| GET | /metrics/prometheus | Metricas Prometheus |
| GET | /docs | Swagger UI |
| GET | /redoc | ReDoc |
| GET | /openapi.json | OpenAPI 3.0 spec |

## Plugins (17 builtin)

### Servicos (core como plugins)

| Plugin | Interface | Status |
|--------|-----------|--------|
| cache | ICache (ResponseCache LRU) | Implementado |
| auth | IAuth (API keys SHA-256) | Implementado |
| rate_limit | IRateLimiter (token bucket) | Implementado |
| request_queue | IRequestQueue (prioridade + workers) | Implementado |

### Pipeline

| Plugin | Descricao | Status |
|--------|-----------|--------|
| logging | Log estruturado de requests/responses | Implementado |
| pii_redactor | Mascara PII (CPF, email, phone, CNPJ) | Implementado |
| semantic_cache | Cache por similaridade semantica (embeddings) | Implementado |
| content_moderation | Filtro de conteudo (wordlists, regex) | Implementado |
| prompt_injection | Deteccao de prompt injection (patterns, scoring) | Implementado |
| response_validator | Validacao de resposta (tamanho) | Parcial |
| rag_injector | Injecao de contexto RAG | Implementado |
| cost_controller | Controle de custo/orcamento por key | Parcial |
| request_router | Roteamento inteligente (model: "auto") | Implementado |
| conversation_memory | Memoria de conversa por sessao | Parcial |
| adaptive_rate_limiter | Rate limiting adaptativo | Parcial |
| streaming_transformer | Transformacao de stream | Stub |
| api_versioning | Metadados de versao | Implementado |
| request_dedup | Deduplicacao por hash | Implementado |
| model_warmup | Estatisticas de warmup | Stub |
| audit | Audit log (JSONL, IAuditSink) | Implementado |
| alerting | Alertas via webhook (IAuditSink) | Implementado |

## Status de Implementacao

| Categoria | Total | Implementado | Parcial | Stub | Rascunho |
|-----------|-------|-------------|---------|------|----------|
| Core Features (RF-01 a RF-12) | 12 | 12 | 0 | 0 | 0 |
| Security Plugins (RF-13 a RF-16) | 4 | 4 | 0 | 0 | 0 |
| Intelligence Plugins (RF-17 a RF-20) | 4 | 4 | 0 | 0 | 0 |
| Operational Plugins (RF-21 a RF-26) | 6 | 6 | 0 | 0 | 0 |
| Compliance Plugins (RF-27 a RF-30) | 4 | 4 | 0 | 0 | 0 |
| **Total** | **30** | **30** | **0** | **0** | **0** |

### Implementadas recentemente

- RF-02 (Usage Tracking): rastreamento por key, quotas, persistencia, endpoints admin
- RF-05 (Prompt Management): system prompt injection, templates, guardrails (block/warn/redact), persistencia completa (key_prompts + templates + guardrails)
- RF-16 (Response Validator): validacao JSON, regras regex, disclaimers (retry automatico documentado como limitacao arquitetural)
- RF-19 (Conversation Memory): user + assistant messages, X-Session-Id, TTL, persistencia em disco via persist_path, endpoints
- RF-20 (Cost Controller): pricing table, budget por key, persistencia, endpoints admin
- RF-21 (Adaptive Rate Limiter): circuit breaker, health monitoring, sliding window
- RF-23 (Streaming Transformer): regex transforms, TTFT metrics, buffering de palavras parciais, flush_buffer no stream end
- RF-26 (Model Warmup): warmup real no startup, keepalive periodico, schedule UTC
- RF-06 (Dashboard Web): SPA embutido, vanilla JS + Chart.js, cards (Uptime, Requests, Active, Memory RSS/Peak, Cache Hit Rate), uptime do servidor
- RF-08 (A/B Testing): weighted routing, metricas p50/p95/p99, X-AB-Variant em streaming, endpoints admin
- RF-09 (Webhook Notifications): fila async, retry backoff, 11 tipos de evento, HTTPS

### Correcoes de bugs pre-existentes

- Dockerfile: libyaml-cpp0.8 corrigido para libyaml-cpp0.7 (Debian Bookworm)
- discovery/configuration_watcher.cpp: RouterPtr qualificado como ConfigurationWatcher::RouterPtr
- discovery/docker_provider.cpp: crypto::sha256 corrigido para crypto::sha256_hex
- discovery/kubernetes_provider.cpp: crypto::sha256 corrigido para crypto::sha256_hex

### Build

- Docker build verificado: imagem `hermes-build-test` criada com sucesso
- Todos os 42 arquivos .cpp compilam sem erros
- Warning restante pre-existente: api_versioning_plugin.cpp (parametro nao usado)

## Estrutura do Projeto

```
src/
  main.cpp                    Ponto de entrada
  gateway.cpp / .h            Servidor HTTP, roteamento, handlers
  config.h                    Configuracao (JSON + env vars)
  plugin.cpp / .h             Plugin interface, PluginManager
  core_services.cpp / .h      ICache, IAuth, IRateLimiter, IRequestQueue, IAuditSink + null impl
  ollama_client.cpp / .h      Cliente Ollama (round-robin, retry)
  provider_router.cpp / .h    Roteamento multi-provider
  ollama_provider.cpp / .h    Provider Ollama
  openai_custom_provider.*    Provider OpenAI/OpenRouter/custom
  request_queue.cpp / .h      Fila com prioridade
  benchmark.cpp / .h          Benchmark ad-hoc
  metrics.h / metrics_*.cpp   Metricas (full + minimal)
  openapi_spec.h              OpenAPI spec embutida
  discovery/                  Discovery dinamico (Docker, K8s, File)
  plugins/                    21 plugins builtin
docs/
  briefing.md                 Briefing do projeto
  ARCHITECTURE.md             Arquitetura detalhada
  COMPARISON_HERMES_VS_ALLTRUE.md  Comparacao com AllTrue.ai
  spec/                       Especificacoes RF-XX (30 FUNCIONAL + 4 AI + 1 UX + 1 ADR)
  specs/                      Documentacao original (RoadMap, historico)
k8s/                          Manifests Kubernetes
scripts/                      Testes, benchmark, stress
```

## Documentacao

| Documento | Descricao |
|-----------|-----------|
| docs/briefing.md | Objetivo, escopo, stack, status |
| docs/ARCHITECTURE.md | Arquitetura, pipeline, builds |
| docs/COMPARISON_HERMES_VS_ALLTRUE.md | Posicionamento vs AllTrue (TRiSM) |
| docs/spec/RF-XX-FUNCIONAL.md | 30 especificacoes funcionais |
| docs/spec/RF-XX-AI.md | 4 especificacoes de componentes AI |
| docs/spec/RF-06-UX.md | Especificacao UX do Dashboard |
| docs/spec/ADR-01-CORE-VS-PLUGIN.md | Decisao arquitetural core vs plugin |
| README.md | Guia de uso completo |

## Builds

| Comando | Descricao |
|---------|-----------|
| make | Build completo |
| make edge | Build minimal (edge/IoT) |
| make debug | Debug symbols |
| make asan | AddressSanitizer |
| make tsan | ThreadSanitizer |
| make run | Compilar e executar |

## CI/CD (GitHub Actions)

6 jobs: build, asan, tsan, edge, docker, docker-edge
