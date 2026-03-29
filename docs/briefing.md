# Briefing — HERMES LLM Gateway

## Objetivo

HERMES (Hybrid Engine for Reasoning, Models & Execution Services) e um gateway HTTP em C++23 que expoe uma API compativel com OpenAI, roteando requisicoes para backends LLM (Ollama, OpenRouter, providers custom). O objetivo e fornecer um ponto de entrada unico, seguro e observavel para consumo de modelos de linguagem, com pipeline de plugins extensivel para seguranca, cache, auditoria e compliance.

## Escopo

### Inclui

- Gateway HTTP compativel com API OpenAI (chat completions, text completions, embeddings, models)
- Roteamento multi-provider com fallback chain (Ollama, OpenRouter, custom OpenAI-compativel)
- Pipeline de plugins extensivel (before_request / after_response)
- Seguranca em runtime: auth por API keys, rate limiting, PII redaction, prompt injection detection, content moderation, response validation
- Cache LRU e semantico (por similaridade de embeddings)
- Fila de requests com prioridade e controle de concorrencia
- Auditoria: audit log (JSONL), alerting (webhook), posture ASPM, compliance reports, security scan
- Observabilidade: metricas JSON e Prometheus, request tracking (UUID), logging estruturado
- Discovery dinamico de backends (Docker, Kubernetes, File)
- Benchmark ad-hoc de modelos (latencia, tokens, score)
- Documentacao OpenAPI embutida (Swagger UI, ReDoc)
- Build edge para ambientes com recursos limitados (IoT, Raspberry Pi)
- Deployment via Docker e Kubernetes (Kustomize)

### Nao inclui

- Plataforma TRiSM completa (discovery de shadow AI, inventario corporativo, governanca de dados)
- Frontend/dashboard web (planejado como RF-06, ainda nao implementado)
- Suporte a protocolos alem de HTTP (gRPC, WebSocket nativo)
- Persistencia em banco de dados (usa arquivos JSON/JSONL)
- Gerenciamento de identidade de usuarios finais (apenas API keys para consumidores do gateway)

## Stack Tecnica

| Aspecto | Tecnologia |
|---------|------------|
| Linguagem | C++23 |
| HTTP Server/Client | cpp-httplib v0.34.0 (header-only) |
| JSON | jsoncpp |
| YAML | yaml-cpp (discovery) |
| TLS | OpenSSL (libssl, libcrypto) |
| HTTP Client (providers) | libcurl |
| Build | Makefile (g++, LTO, -O3) |
| Container | Docker (Debian 12-slim, multi-stage) |
| Orquestracao | Kubernetes (Kustomize) |
| CI/CD | GitHub Actions (build, asan, tsan, edge, docker) |

## Arquitetura

### Core minimo + plugins

O HERMES separa o core minimo (gateway, config, logger, interfaces) das implementacoes plugaveis. Em build completo, servicos como cache, auth e rate limiting sao fornecidos por plugins builtin. Em build edge, usam implementacoes null (sem alocacao, sem overhead).

### Pipeline de plugins

Requests passam por um pipeline configuravel:
1. `before_request` — plugins processam na ordem do pipeline (podem bloquear, modificar ou retornar cache)
2. Backend LLM — request e enviada ao provider resolvido
3. `after_response` — plugins processam na ordem inversa
4. `notify_request_completed` — plugins com IAuditSink recebem o evento para auditoria

### Providers

- OllamaProvider — comunicacao com backends Ollama (round-robin, health check, retry)
- OpenAICustomProvider — proxy para qualquer API OpenAI-compativel (OpenRouter, Azure, custom)

### Builds

- **Full** (`make`) — todas as features, plugins e documentacao
- **Edge** (`make edge`) — core minimo, sem cache/auth/rate/queue/benchmark/docs/multi-provider

## Posicionamento

O HERMES cobre parte do que plataformas TRiSM como AllTrue.ai oferecem em runtime protection e detection, mas nao se propoe a substituir uma plataforma completa. E um componente de infraestrutura que pode ser usado dentro de um ecossistema maior, fornecendo dados de trafego e eventos para ferramentas de governanca e compliance.

Alinhamento com pilares TRiSM:
- **Runtime Protection** — forte (auth, rate limit, PII, prompt injection, moderation, validation)
- **Detection & Response** — implementado (audit log, alerting, metricas, request tracking)
- **Compliance** — dados disponiveis (audit, compliance report, posture ASPM, security scan)
- **Visibility** — limitada ao gateway (modelos, keys, plugins, metricas)
- **ASPM** — config manual + endpoint de postura
- **Security Testing** — protecao em runtime + security scan proativo

## Status Atual de Implementacao

### Totalmente implementado (23)

- Gateway HTTP com API OpenAI-compativel
- Multi-provider (Ollama + OpenAI custom/OpenRouter)
- Request Queue com prioridade
- Plugin/Middleware System (pipeline before/after)
- Dynamic Discovery (Docker, K8s, File)
- Benchmark ad-hoc
- Cache LRU + Semantic Cache
- Auth (API keys SHA-256), Rate Limiting, PII Redaction
- Content Moderation, Prompt Injection Detector
- RAG Injector, Request Router (auto)
- Logging, Request Deduplication, API Versioning
- Audit Log (JSONL + query), Alerting (webhook)
- Posture ASPM, Compliance Report, Security Scan

### Parcialmente implementado (4)

- Response Validator (validate_json_object nao utilizado)
- Conversation Memory (so armazena mensagens do assistant)
- Cost Controller (estimate_cost retorna valor fixo, sem persistencia)
- Adaptive Rate Limiter (nao integra com IRateLimiter real)

### Stubs (2)

- Streaming Transformer (before/after retornam Continue sem logica)
- Model Warmup (nao faz warmup real no startup)

### Nao implementado (5 features planejadas)

- Usage Tracking & Quotas (RF-02)
- Prompt Management (RF-05)
- Dashboard Web (RF-06)
- A/B Testing (RF-08)
- Webhook Notifications (RF-09)

## Restricoes e decisoes arquiteturais

- Sem dependencias externas de runtime alem de libjsoncpp, libssl, libcurl e libyaml-cpp
- Header-only para HTTP (cpp-httplib) — zero overhead de linking
- SHA-256 implementacao propria (sem dependencia de libcrypto para hash de keys)
- Persistencia em filesystem (config.json, keys.json, audit JSONL) — sem banco de dados
- Thread-safety via mutex e atomics (std::jthread para workers)
- Plugins sao builtin (compilados junto); suporte a plugins dinamicos (.so) planejado

## Referencia de documentacao

| Documento | Descricao |
|-----------|-----------|
| `docs/ARCHITECTURE.md` | Arquitetura, core vs plugins, pipeline, builds |
| `docs/COMPARISON_HERMES_VS_ALLTRUE.md` | Comparacao com AllTrue.ai por pilares TRiSM |
| `docs/spec/` | Especificacoes no formato RF-XX (FUNCIONAL, UX, AI) |
| `README.md` | Guia de uso, endpoints, configuracao, build |
| `README.md` | Guia de uso, endpoints, configuracao, build |
