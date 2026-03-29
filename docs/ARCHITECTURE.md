# Arquitetura do HERMES

Visao geral do sistema, componentes e fluxos. Especificacoes detalhadas em [spec/](spec/).

---

## Visao de alto nivel

```mermaid
flowchart TB
    subgraph clients [Clientes]
        SDK[OpenAI SDK]
        Curl[curl / HTTP]
    end
    subgraph gateway [HERMES]
        Server[httplib Server]
        Auth[IAuth]
        RateLimit[IRateLimiter]
        Cache[ICache]
        Queue[IRequestQueue]
        Metrics[IMetricsCollector]
        PluginMgr[PluginManager]
        Pipeline[Pipeline before/after]
    end
    subgraph backends [Backends]
        Ollama[Ollama]
        OpenRouter[OpenRouter]
        Custom[Custom API]
    end
    SDK --> Server
    Curl --> Server
    Server --> Auth
    Server --> RateLimit
    Server --> PluginMgr
    PluginMgr --> Pipeline
    Server --> Cache
    Server --> Queue
    Server --> Metrics
    Pipeline --> Ollama
    Pipeline --> OpenRouter
    Pipeline --> Custom
```

---

## Core vs servicos plugaveis

```mermaid
flowchart LR
    subgraph core [Core minimo]
        GW[Gateway]
        Config[Config]
        Logger[Logger]
        OllamaClient[OllamaClient]
        PluginManager[PluginManager]
        Interfaces[ICache, IAuth, IRateLimiter, IRequestQueue, IMetricsCollector]
    end
    subgraph impl [Implementacoes]
        Null[Null impl - edge]
        Plugins[Plugins builtin - full]
    end
    GW --> Interfaces
    Interfaces --> Null
    Interfaces --> Plugins
```

| Componente | Build full | Build edge |
|------------|------------|------------|
| Cache | CachePlugin (ResponseCache) | NullCache |
| Auth | AuthPlugin (ApiKeyManager) | NullAuth |
| Rate limit | RateLimitPlugin | NullRateLimiter |
| Request queue | RequestQueuePlugin | NullRequestQueue |
| Metricas | MetricsCollector (full + Prometheus) | MinimalMetricsCollector |
| Update check | UpdateChecker | Desabilitado |

---

## Pipeline de plugins

```mermaid
sequenceDiagram
    participant Req as Request
    participant GW as Gateway
    participant PM as PluginManager
    participant P1 as Plugin 1
    participant P2 as Plugin 2
    participant Backend as Backend

    Req->>GW: POST /v1/chat/completions
    GW->>PM: run_before_request(body, ctx)
    PM->>P1: before_request
    P1-->>PM: Continue / Block / cached_response
    PM->>P2: before_request
    P2-->>PM: Continue
    PM-->>GW: PluginResult
    alt Block ou cached_response
        GW-->>Req: 4xx ou response cacheada
    else Continue
        GW->>Backend: chat completion
        Backend-->>GW: response
        GW->>PM: run_after_response(response, ctx)
        PM->>P2: after_response
        P2-->>PM: Continue
        PM->>P1: after_response
        P1-->>PM: Continue
        GW-->>Req: 200 + response
    end
```

Ordem: `before_request` na ordem do pipeline; `after_response` na ordem inversa.

Apos `run_after_response`, o gateway chama `PluginManager::notify_request_completed(entry)` com um `AuditEntry`. Plugins que implementam `IAuditSink` (ex.: plugin `audit`) recebem o evento de forma thread-safe e nao bloqueante (implementacoes usam fila + worker).

---

## Detection & Response e Compliance

- **Core:** Emite o evento "request completed" (`AuditEntry`) apos cada request via `notify_request_completed`. Nao persiste nem expõe endpoints de auditoria.
- **Plugin audit:** Implementa `IAuditSink` e `IAuditQueryProvider`; persiste em JSONL (e futuramente Syslog, HTTP, S3); expõe consulta via `GET /admin/audit`.
- **Plugins futuros:** Alerting e compliance reports podem consumir o mesmo evento (como sinks) ou ler dados do plugin audit. Ver [ADR-01-CORE-VS-PLUGIN.md](spec/ADR-01-CORE-VS-PLUGIN.md).

---

## Build edge vs build completo

```mermaid
flowchart TB
    subgraph full [Build completo - make]
        F_GW[Gateway]
        F_Cache[CachePlugin]
        F_Auth[AuthPlugin]
        F_Rate[RateLimitPlugin]
        F_Queue[RequestQueuePlugin]
        F_Metrics[MetricsCollector full]
        F_Update[UpdateChecker]
        F_Bench[Benchmark]
        F_Docs[OpenAPI]
        F_Multi[Multi-Provider]
    end
    subgraph edge [Build edge - make edge]
        E_GW[Gateway]
        E_Null[Null Cache/Auth/RateLimit/Queue]
        E_MinMetrics[MinimalMetricsCollector]
    end
    full -->|EDGE_CORE=1, exclui .cpp| edge
```

Flags: `EDGE_CORE=1`, `BENCHMARK_ENABLED=0`, `DOCS_ENABLED=0`, `MULTI_PROVIDER=0`.

---

## Especificacoes por topico

| RF | Doc | Descricao |
|----|-----|-----------|
| RF-01 | [RF-01-FUNCIONAL.md](spec/RF-01-FUNCIONAL.md) | Roteamento multi-provider, fallback |
| RF-03 | [RF-03-FUNCIONAL.md](spec/RF-03-FUNCIONAL.md) | Fila de requests com prioridade |
| RF-04 | [RF-04-FUNCIONAL.md](spec/RF-04-FUNCIONAL.md) | Audit log (core + plugin) |
| RF-07 | [RF-07-FUNCIONAL.md](spec/RF-07-FUNCIONAL.md) | Cache por similaridade semantica |
| RF-10 | [RF-10-FUNCIONAL.md](spec/RF-10-FUNCIONAL.md) | Sistema de plugins (pipeline) |
| RF-13 | [RF-13-FUNCIONAL.md](spec/RF-13-FUNCIONAL.md) | Mascaramento de PII |
| RF-15 | [RF-15-FUNCIONAL.md](spec/RF-15-FUNCIONAL.md) | Deteccao de prompt injection |
| RF-27 | [RF-27-FUNCIONAL.md](spec/RF-27-FUNCIONAL.md) | Alerting (webhook, regras 4xx/blocked) |
| RF-28 | [RF-28-FUNCIONAL.md](spec/RF-28-FUNCIONAL.md) | Relatorio de postura ASPM |
| RF-29 | [RF-29-FUNCIONAL.md](spec/RF-29-FUNCIONAL.md) | Relatorios de compliance |
| RF-30 | [RF-30-FUNCIONAL.md](spec/RF-30-FUNCIONAL.md) | Testes de seguranca proativos |
| ADR-01 | [ADR-01-CORE-VS-PLUGIN.md](spec/ADR-01-CORE-VS-PLUGIN.md) | Core vs plugin (AllTrue TRiSM) |

Todas as especificacoes em [spec/](spec/).
