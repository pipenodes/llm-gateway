# HERMES

[![CI/CD](https://github.com/pipenodes/llm-gateway/actions/workflows/deploy.yml/badge.svg)](https://github.com/pipenodes/llm-gateway/actions/workflows/deploy.yml)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://github.com/pipenodes/llm-gateway/blob/master/LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-23-00599C?logo=c%2B%2B&logoColor=white)](https://en.cppreference.com/w/cpp/23)
[![Docker](https://img.shields.io/badge/GHCR-image-2496ED?logo=docker&logoColor=white)](https://github.com/pipenodes/llm-gateway/pkgs/container/llm-gateway)
[![Kubernetes](https://img.shields.io/badge/Kubernetes-manifests-326CE5?logo=kubernetes&logoColor=white)](https://github.com/pipenodes/llm-gateway/tree/master/k8s)
[![Kustomize](https://img.shields.io/badge/Kustomize-deploy-30638e?logo=kustomize&logoColor=white)](https://github.com/pipenodes/llm-gateway/blob/master/k8s/kustomization.yaml)
[![GitHub stars](https://img.shields.io/github/stars/pipenodes/llm-gateway?style=flat&logo=github)](https://github.com/pipenodes/llm-gateway/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/pipenodes/llm-gateway?style=flat&logo=github)](https://github.com/pipenodes/llm-gateway/forks)

[![Artifact attestations](https://img.shields.io/badge/Proveni%C3%AAncia-Artifact_Attestations-2088FF?logo=github)](https://github.com/pipenodes/llm-gateway/attestations)
[![SLSA](https://img.shields.io/badge/SLSA-build%20provenance-3FB950?logo=slsa&logoColor=white)](https://slsa.dev/)
[![Sigstore](https://img.shields.io/badge/Sigstore-assinatura-4445ED?logo=sigstore&logoColor=white)](https://www.sigstore.dev/)
[![Prometheus](https://img.shields.io/badge/M%C3%A9tricas-Prometheus-E6522C?logo=prometheus&logoColor=white)](https://prometheus.io/docs/introduction/overview/)
[![OpenAPI](https://img.shields.io/badge/OpenAPI-documenta%C3%A7%C3%A3o-6BA539?logo=openapiinitiative&logoColor=white)](https://www.openapis.org/)
[![Ollama](https://img.shields.io/badge/Backend-Ollama-000000?logo=ollama&logoColor=white)](https://ollama.com/)

<sub>**Deploy:** manifests em [`k8s/`](https://github.com/pipenodes/llm-gateway/tree/master/k8s) (Kubernetes + [Kustomize](https://kubectl.docs.kubernetes.io/references/kustomize/kustomization/)), aplicados no CI em `main`/`master`. **Proveniência:** o workflow [CI/CD](https://github.com/pipenodes/llm-gateway/actions/workflows/deploy.yml) gera [Artifact Attestations](https://docs.github.com/pt/actions/security-guides/using-artifact-attestations-to-establish-provenance-for-builds) para a imagem no GHCR (`actions/attest-build-provenance`); ver [attestations do repositório](https://github.com/pipenodes/llm-gateway/attestations) e validação com `gh attestation verify`.</sub>

HERMES (Hybrid Engine for Reasoning, Models & Execution Services) — Gateway HTTP em C++23 que expoe uma API compativel com OpenAI, roteando requisicoes para backends [Ollama](https://ollama.com). Inclui cache, rate limiting, API keys, streaming SSE, metricas Prometheus, documentacao OpenAPI e mais.

## Inicio Rapido

```bash
# 1. Clone o repositorio
git clone <repo-url> && cd hermes

# 2. Configure o docker-compose.yml (veja secao Configuracao)

# 3. Suba o gateway
docker compose up -d --build

# 4. Teste
curl http://localhost:8080/health
# {"status":"ok"}

# 5. Chat completion
curl -X POST http://localhost:8080/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{"model":"llama3:8b","messages":[{"role":"user","content":"Hello"}]}'
```

## Arquitetura

```mermaid
flowchart TB
    subgraph client [Cliente]
        SDK[OpenAI SDK / curl]
    end
    subgraph gateway [HERMES :8080]
        HTTP[httplib Server]
        Auth[Auth API Keys]
        RateLimit[Rate Limiting]
        Cache[Cache LRU]
        Aliases[Model Aliasing]
        Stream[Streaming SSE]
        Metrics[Metricas]
        HTTP --> Auth
        HTTP --> RateLimit
        HTTP --> Cache
        HTTP --> Aliases
        HTTP --> Stream
        HTTP --> Metrics
    end
    subgraph backends [Backends]
        O1[Ollama :11434]
        O2[Ollama ...]
    end
    SDK -->|POST /v1/chat/completions| HTTP
    HTTP -->|round-robin + health check| O1
    HTTP --> O2
```

```mermaid
flowchart LR
    Client[Cliente] -->|"1. Request"| Gateway[Gateway]
    Gateway -->|"2. Pipeline before_request"| Plugins[Plugins]
    Plugins -->|"3. Backend"| Ollama[Ollama]
    Ollama -->|"4. Response"| Gateway
    Gateway -->|"5. Pipeline after_response"| Plugins
    Plugins -->|"6. Resposta"| Client
```

## Endpoints

### API OpenAI-Compativel

| Metodo | Path | Descricao |
|--------|------|-----------|
| `POST` | `/v1/chat/completions` | Chat completions (streaming e non-streaming) |
| `POST` | `/v1/completions` | Text completions (legacy) |
| `POST` | `/v1/embeddings` | Geracao de embeddings |
| `GET` | `/v1/models` | Lista modelos disponiveis + aliases |
| `GET` | `/v1/models/{model}` | Detalhes de um modelo |

### Administracao (requer `ADMIN_KEY`)

| Metodo | Path | Descricao |
|--------|------|-----------|
| `POST` | `/admin/keys` | Criar API key |
| `GET` | `/admin/keys` | Listar API keys |
| `DELETE` | `/admin/keys/{alias}` | Revogar API key |
| `GET` | `/admin/plugins` | Listar plugins (nome, versao, tipo) |
| `GET` | `/admin/audit` | Consultar audit log (requer plugin audit; filtros: key_alias, model, from, to, status, limit, offset) |
| `GET` | `/admin/posture` | Relatorio de postura (config snapshot, plugins, modelos, checks) para ASPM |
| `GET` | `/admin/compliance/report` | Relatorio de uso agregado (requer plugin audit; from, to, format=json\|csv, group_by=key_alias\|model) |
| `POST` | `/admin/security-scan` | Testes de seguranca proativos (body opcional: `{"model":"..."}`; prompt injection, relatorio pass/fail) |
| `GET` | `/admin/queue` | Estado da fila de requests |
| `DELETE` | `/admin/queue` | Limpar fila pendente |
| `GET` | `/admin/updates/check` | Verificar atualizacoes (core e plugins) |
| `POST` | `/admin/updates/apply` | Baixar binario para path configurado (opcional: processo sai com codigo para supervisor reiniciar) |

### Sistema

| Metodo | Path | Descricao |
|--------|------|-----------|
| `GET` | `/health` | Health check |
| `GET` | `/metrics` | Metricas em JSON |
| `GET` | `/metrics/prometheus` | Metricas em formato Prometheus |
| `GET` | `/docs` | Swagger UI |
| `GET` | `/redoc` | ReDoc UI |
| `GET` | `/openapi.json` | Especificacao OpenAPI 3.0 |

## Configuracao

Toda configuracao pode ser feita via `config.json` ou variaveis de ambiente (env vars sobrescrevem o JSON).

### Variaveis de Ambiente

| Variavel | Descricao | Default |
|----------|-----------|---------|
| `OLLAMA_URL` | URLs dos backends Ollama (separadas por `,`) | `http://localhost:11434` |
| `ADMIN_KEY` | Chave para endpoints `/admin/*` | (vazio = admin desabilitado) |
| `ADMIN_IP_WHITELIST` | IPs/CIDR permitidos para admin (separados por `,`) | (vazio = todos) |
| `MODEL_ALIASES` | Aliases de modelos (`alias1=model1,alias2=model2`) | (vazio) |
| `RATE_LIMIT` | Rate limit global em RPM | `0` (desabilitado) |
| `CACHE_ENABLED` | Habilitar cache | `false` |
| `CACHE_TTL` | TTL do cache em segundos | `300` |
| `CACHE_MAX_ENTRIES` | Max entradas no cache | `1000` |
| `CACHE_MAX_MEMORY_MB` | Limite de memoria do cache em MB | `0` (ilimitado) |
| `DOCS_ENABLED` | Habilitar Swagger/ReDoc | `false` |
| `CORS_ORIGIN` | `Access-Control-Allow-Origin` | `*` |
| `MAX_PAYLOAD_MB` | Tamanho maximo do payload em MB | `1` |
| `UPDATE_CHECK_URL` | URL do manifest JSON para verificacao de atualizacoes (core e plugins) | (vazio = desabilitado) |
| `UPDATE_STAGED_BINARY_PATH` | Caminho onde gravar o binario baixado (ex: `/tmp/hermes.new`) para apply | (vazio = apply desabilitado) |
| `UPDATE_EXIT_CODE_FOR_RESTART` | Codigo de saida apos apply para o supervisor reiniciar (0 = nao sair) | `0` |

### config.json

```json
{
  "gateway": { "port": 8080, "host": "0.0.0.0" },
  "ollama": { "url": "http://localhost:11434" },
  "model_aliases": {
    "gpt-4": "qwen3:8b",
    "gpt-3.5-turbo": "gemma3:4b",
    "text-embedding-3-small": "nomic-embed-text:latest"
  },
  "cache": {
    "enabled": false,
    "ttl": 300,
    "max_entries": 1000,
    "max_memory_mb": 128
  },
  "docs": { "enabled": false },
  "git aupdates": {
    "check_url": "https://seu-servidor.com/releases/manifest.json",
    "staged_binary_path": "/tmp/hermes.new",
    "exit_code_for_restart": 42
  }
}
```

### Auto-update (verificacao e apply)

O gateway pode **verificar** se ha novas versoes do **core** e dos **plugins** consultando um manifest JSON e, opcionalmente, **baixar** o novo binario e **sinalizar reinicio** para um supervisor.

- **Configuracao:** defina `updates.check_url` no `config.json` ou `UPDATE_CHECK_URL` com a URL do manifest.
- **Check:** `GET /admin/updates/check` (requer `ADMIN_KEY`). Resposta inclui:
  - `core`: `current`, `latest` (se houver), `available` (boolean), `release_notes`, `download_url` (opcional).
  - `plugins`: array com `name`, `current`, `latest`, `available`, `download_url` (opcional).
  - `error` e `message`: se a URL nao estiver configurada ou houver falha de rede/parse.
- **Apply:** `POST /admin/updates/apply` com body `{ "download_url": "https://..." }`. Requer `updates.staged_binary_path` (ou `UPDATE_STAGED_BINARY_PATH`). O gateway baixa o binario e grava em modo atomico no path configurado (ex: `/tmp/hermes.new`). Opcionalmente, se `updates.exit_code_for_restart` (ou `UPDATE_EXIT_CODE_FOR_RESTART`) for diferente de 0, apos enviar a resposta HTTP o processo **espera 2 segundos e sai** com esse codigo, para um supervisor substituir o binario e reiniciar.

**Formato esperado do manifest (JSON servido na URL):**

```json
{
  "core": {
    "version": "0.0.5",
    "release_notes": "Correcoes e melhorias.",
    "download_url": "https://releases.example.com/hermes/v0.0.5/hermes-linux-amd64"
  },
  "plugins": [
    { "name": "semantic_cache", "version": "1.0.1", "download_url": "https://..." }
  ]
}
```

Comparacao de versoes e feita por semver (major.minor.patch). Plugins atuais sao os retornados por `GET /admin/plugins` (built-in).

#### Reload do binario (como o processo e substituido)

O processo em execucao **nao pode se substituir** sozinho; e preciso um **supervisor** (script, systemd, etc.) que:

1. **Opcao A – Manual:** Voce chama `POST /admin/updates/apply` (ou um script que chama check, pega `download_url` e chama apply). O binario novo fica em `staged_binary_path`. Depois o script para o gateway (ex: `systemctl stop` ou `kill`), substitui o binario pelo arquivo em staged (ex: `mv /tmp/hermes.new /app/hermes`), e inicia de novo.

2. **Opcao B – Exit code + supervisor:** Configure `staged_binary_path` e `exit_code_for_restart` (ex: `42`). Ao chamar `POST /admin/updates/apply`, o gateway baixa o binario, responde 200 e, 2 segundos depois, **sai com codigo 42**. O supervisor deve:
   - ao ver **exit code 42**: copiar/substituir o binario em execucao pelo arquivo em `staged_binary_path` e reiniciar o servico (ex: systemd `Restart=on-failure` so nao basta; use um **wrapper** que, apos exit 42, faz o replace e exec do novo binario).

Exemplo de **wrapper** para reload automatico (Linux):

```bash
#!/bin/bash
BINARY="/app/hermes"
STAGED="/tmp/hermes.new"
while true; do
  "$BINARY" "$@"
  code=$?
  if [ "$code" -eq 42 ] && [ -f "$STAGED" ]; then
    mv -f "$STAGED" "$BINARY" && chmod +x "$BINARY"
    exec "$BINARY" "$@"
  fi
  exit $code
done
```

Com systemd, use `ExecStart=/opt/hermes/wrapper.sh` (e `staged_binary_path=/tmp/hermes.new`, `exit_code_for_restart=42`).

### docker-compose.yml

```yaml
services:
  hermes:
    build: .
    ports:
      - "8080:8080"
    restart: unless-stopped
    environment:
      - OLLAMA_URL=http://host.docker.internal:11434
      - ADMIN_KEY=your-admin-secret
      - ADMIN_IP_WHITELIST=0.0.0.0/0
      - RATE_LIMIT=60
      - MODEL_ALIASES=gpt-4=qwen3:8b,gpt-3.5-turbo=gemma3:4b
      - CACHE_ENABLED=true
      - CACHE_TTL=300
      - CACHE_MAX_ENTRIES=1000
      - CACHE_MAX_MEMORY_MB=128
      - DOCS_ENABLED=true
      - CORS_ORIGIN=*
      - MAX_PAYLOAD_MB=4
    volumes:
      - gateway-data:/app/data

volumes:
  gateway-data:
```

## Uso

### Subir o Gateway

```bash
docker compose up -d --build
```

### Chat Completion

```bash
curl -X POST http://localhost:8080/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "gpt-4",
    "messages": [{"role": "user", "content": "Hello!"}],
    "temperature": 0.7,
    "max_tokens": 100
  }'
```

### Chat Completion com Streaming

```bash
curl -X POST http://localhost:8080/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "gpt-4",
    "messages": [{"role": "user", "content": "Tell me a story"}],
    "stream": true
  }'
```

### Embeddings

```bash
curl -X POST http://localhost:8080/v1/embeddings \
  -H "Content-Type: application/json" \
  -d '{
    "model": "text-embedding-3-small",
    "input": "The quick brown fox"
  }'
```

### Listar Modelos

```bash
curl http://localhost:8080/v1/models
```

### Tool Calling

```bash
curl -X POST http://localhost:8080/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "gpt-4",
    "messages": [{"role": "user", "content": "What is the weather in SF?"}],
    "tools": [{
      "type": "function",
      "function": {
        "name": "get_weather",
        "description": "Get weather for a city",
        "parameters": {
          "type": "object",
          "properties": {
            "city": {"type": "string"}
          },
          "required": ["city"]
        }
      }
    }]
  }'
```

### JSON Mode

```bash
curl -X POST http://localhost:8080/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "gpt-4",
    "messages": [{"role": "user", "content": "List 3 colors as JSON"}],
    "response_format": {"type": "json_object"}
  }'
```

## Gerenciamento de API Keys

### Criar Key

```bash
curl -X POST http://localhost:8080/admin/keys \
  -H "Authorization: Bearer $ADMIN_KEY" \
  -H "Content-Type: application/json" \
  -d '{
    "alias": "dev-team",
    "rate_limit_rpm": 30,
    "ip_whitelist": ["10.0.0.0/8"]
  }'
```

Resposta (a key completa so aparece uma vez):

```json
{
  "key": "sk-aBcDeFgHiJkLmNoPqRsTuVwXyZ...",
  "alias": "dev-team",
  "created_at": 1740355200
}
```

### Usar Key

```bash
curl -X POST http://localhost:8080/v1/chat/completions \
  -H "Authorization: Bearer sk-aBcDeFgHiJkLmNoPqRsTuVwXyZ..." \
  -H "Content-Type: application/json" \
  -d '{"model":"gpt-4","messages":[{"role":"user","content":"Hi"}]}'
```

### Listar Keys

```bash
curl http://localhost:8080/admin/keys \
  -H "Authorization: Bearer $ADMIN_KEY"
```

### Revogar Key

```bash
curl -X DELETE http://localhost:8080/admin/keys/dev-team \
  -H "Authorization: Bearer $ADMIN_KEY"
```

## Funcionalidades

### Cache de Respostas

Cache LRU em memoria para respostas deterministicas:

- Chat completions com `temperature=0` (sem tools, sem response_format)
- Embeddings (sempre cacheadas)
- Headers `X-Cache: HIT | MISS | BYPASS`
- Configuravel via `CACHE_ENABLED`, `CACHE_TTL`, `CACHE_MAX_ENTRIES`, `CACHE_MAX_MEMORY_MB`

### Rate Limiting

Token bucket por IP ou por API key:

- Rate limit global via `RATE_LIMIT` (RPM)
- Rate limit por key via campo `rate_limit_rpm` na criacao da key
- Headers padrao OpenAI: `x-ratelimit-limit-requests`, `x-ratelimit-remaining-requests`, `x-ratelimit-reset-requests`
- Resposta `429 Too Many Requests` com header `Retry-After`

### Multi-Backend Ollama

Load balancing round-robin com failover:

- Multiplos backends via `OLLAMA_URL=http://gpu1:11434,http://gpu2:11434`
- Health checks automaticos a cada 30s
- Skip de backends unhealthy
- Retry com backoff exponencial (200ms, 400ms, 800ms)
- Connection pool thread-local (reutiliza conexoes HTTP)

### Model Aliasing

Mapeamento transparente de nomes:

```
MODEL_ALIASES=gpt-4=qwen3:8b,gpt-3.5-turbo=gemma3:4b
```

Clientes OpenAI SDK podem usar `gpt-4` e o gateway roteia para `qwen3:8b`.

### Streaming SSE

Server-Sent Events compativeis com formato OpenAI:

- Chat completions (`/v1/chat/completions` com `stream: true`)
- Text completions (`/v1/completions` com `stream: true`)
- Suporte a tool calls em streaming
- Cancelamento de streams

### Seguranca

- API keys armazenadas como hash SHA-256 (nunca em texto plano)
- Comparacao constante-time para prevenir timing attacks
- IP whitelist com suporte a CIDR (IPv4) por key e para admin
- Payload maximo configuravel
- CORS configuravel

### Request Tracking

- `X-Request-Id` gerado automaticamente (UUID v4)
- Echo de `X-Request-Id` ou `X-Client-Request-Id` do cliente
- Request ID incluido em todos os logs estruturados

### Metricas

**JSON** (`GET /metrics`):

```json
{
  "uptime_seconds": 3600,
  "memory_rss_kb": 12048,
  "memory_peak_kb": 14200,
  "requests_total": 542,
  "requests_active": 2,
  "cache": {
    "enabled": true,
    "entries": 150,
    "hits": 1200,
    "misses": 340,
    "hit_rate": 0.779
  }
}
```

**Prometheus** (`GET /metrics/prometheus`):

```
gateway_requests_total 542
gateway_requests_active 2
gateway_memory_rss_bytes 12337152
gateway_uptime_seconds 3600
gateway_cache_hits 1200
gateway_cache_misses 340
```

### Documentacao

Swagger UI e ReDoc embutidos (sem dependencias externas):

- `GET /docs` -- Swagger UI
- `GET /redoc` -- ReDoc
- `GET /openapi.json` -- Spec OpenAPI 3.0

Habilitar via `DOCS_ENABLED=true`.

## Build Local

### Pre-requisitos

```bash
sudo apt-get install build-essential libjsoncpp-dev wget
```

### Compilar

```bash
make              # build completo (benchmark, docs, multi-provider, queue)
make debug        # build com debug symbols
make asan         # build com AddressSanitizer
make tsan         # build com ThreadSanitizer
make run          # compilar e executar
```

**Windows:** o Makefile usa `g++` e ferramentas Unix; para compilar no Windows use **WSL** (Ubuntu) ou construa via Docker (`docker compose up -d --build`). Em WSL: instale os pre-requisitos com `apt-get` e execute `make` no diretorio do projeto.

### Testes manuais (endpoints admin)

Com o gateway em execucao e `ADMIN_KEY` configurado, pode validar os endpoints de administracao:

```bash
# Health
curl -s http://localhost:8080/health

# Postura (config snapshot, plugins, modelos)
curl -s -H "Authorization: Bearer $ADMIN_KEY" http://localhost:8080/admin/posture

# Audit log (requer plugin audit; filtros: key_alias, model, from, to, status, limit, offset)
curl -s -H "Authorization: Bearer $ADMIN_KEY" "http://localhost:8080/admin/audit?limit=5"

# Relatorio de compliance (requer plugin audit; from, to, format=json|csv, group_by=key_alias|model)
curl -s -H "Authorization: Bearer $ADMIN_KEY" "http://localhost:8080/admin/compliance/report?format=json"

# Security scan (testes proativos de prompt injection; body opcional: {"model":"..."})
curl -s -X POST -H "Authorization: Bearer $ADMIN_KEY" -H "Content-Type: application/json" \
  http://localhost:8080/admin/security-scan
```

### Build Edge (minimal para edge computing)

Para ambientes com poucos recursos (Raspberry Pi, IoT, VMs pequenas), use o build edge: binario menor, menos RAM e sem componentes opcionais.

```mermaid
flowchart LR
    subgraph full [Build completo]
        F1[Gateway]
        F2[CachePlugin]
        F3[AuthPlugin]
        F4[RateLimitPlugin]
        F5[RequestQueuePlugin]
        F6[MetricsCollector full]
        F7[UpdateChecker]
        F8[Benchmark]
        F9[OpenAPI]
        F10[Multi-Provider]
    end
    subgraph edge [Build edge]
        E1[Gateway]
        E2[NullCache]
        E3[NullAuth]
        E4[NullRateLimiter]
        E5[NullRequestQueue]
        E6[MinimalMetricsCollector]
    end
    full -.->|"make edge"| edge
```

```bash
make edge         # mesmo binario hermes, compilado em modo minimal
```

**Incluido no edge:** HTTP server, Config, Logger, OllamaClient, PluginManager, interfaces de servico (ICache, IAuth, IRateLimiter, IRequestQueue) com implementacoes null — sem alocacao de cache, API keys, rate limiter nem fila. Metricas minimas (uptime + contadores). Sem update checker.

**Excluido no edge:** Benchmark (`/v1/benchmark`), OpenAPI/Swagger/ReDoc, Multi-Provider (apenas Ollama), Request Queue e os quatro plugins de servico (cache, auth, rate_limit, request_queue). Update checker e metricas full (Prometheus completo, readProcMem). O core permanece minimo; em build normal esses servicos sao fornecidos por plugins builtin registrados na inicializacao.

Flags de compilacao (em `src/edge_config.h`): `EDGE_CORE=1`, `BENCHMARK_ENABLED=0`, `DOCS_ENABLED=0`, `MULTI_PROVIDER=0`.

Para validar apos `make edge`: subir o gateway e rodar `bash scripts/stress/stress_test.sh http://localhost:8080 256 24` ou `bash scripts/test/test_gateway.sh http://localhost:8080`.

**Imagem Docker edge:** `docker build -f Dockerfile.edge -t hermes:edge .`

### Plugins

O gateway usa um pipeline de plugins configurado em `config.json` em `plugins.pipeline`. Cada entrada pode ter `name`, `enabled` e `config`.

```mermaid
flowchart TB
    subgraph core [Core minimo]
        GW[Gateway]
        IC[ICache]
        IA[IAuth]
        IR[IRateLimiter]
        IQ[IRequestQueue]
        IM[IMetricsCollector]
    end
    subgraph services [Servicos - plugins builtin]
        CP[CachePlugin]
        AP[AuthPlugin]
        RP[RateLimitPlugin]
        QP[RequestQueuePlugin]
    end
    subgraph pipeline [Pipeline de plugins]
        P1[logging]
        P2[pii_redactor]
        P3[semantic_cache]
        P4[content_moderation]
        P5[...]
    end
    GW --> IC
    GW --> IA
    GW --> IR
    GW --> IQ
    GW --> IM
    CP -.->|implementa| IC
    AP -.->|implementa| IA
    RP -.->|implementa| IR
    QP -.->|implementa| IQ
    GW --> P1
    P1 --> P2
    P2 --> P3
    P3 --> P4
    P4 --> P5
```

**Servicos (core como plugins, apenas em build nao-edge):** `cache`, `auth`, `rate_limit`, `request_queue` — registrados automaticamente a partir da config (cache, rate_limit, queue, keys). Em build edge nao sao registrados; o gateway usa implementacoes null.

**Plugins de pipeline (habilitados por nome no pipeline):**

| Nome | Descricao | Tier |
|------|-----------|------|
| `logging` | Log estruturado de requests/responses | core |
| `semantic_cache` | Cache por similaridade semantica (embeddings) | core |
| `pii_redactor` | Mascara PII em mensagens e restaura na resposta | core |
| `content_moderation` | Filtra conteudo inadequado (input/output) | core |
| `prompt_injection` | Detecta e bloqueia tentativas de prompt injection | core |
| `response_validator` | Valida resposta (tamanho, regras) | core |
| `rag_injector` | Injeta contexto RAG no prompt (ex.: `rag_context` no body) | core |
| `cost_controller` | Controla custo/orcamento por key | core |
| `request_router` | Roteia por `model: "auto"` (regras por conteudo) | core |
| `conversation_memory` | Memoria de conversa por sessao | core |
| `adaptive_rate_limiter` | Limites adaptativos por backend health e circuit breaker | core |
| `streaming_transformer` | Transforma stream (placeholder) | core |
| `api_versioning` | Metadados de versao da API | core |
| `request_dedup` | Deduplicacao de requests (cache por hash) | core |
| `model_warmup` | Estatisticas de warmup | core |
| `audit` | Audit log (IAuditSink): persiste eventos request em JSONL; GET /admin/audit para consulta | core |
| `alerting` | Alertas (IAuditSink): regras por 4xx/blocked; webhook HTTP POST assincrono | core |
| `guardrails` | LLM Firewall L1/L2/L3: payload, allow/deny models, rate limit por tenant, ML classifier, LLM Judge async; politicas por tenant+app | **enterprise** |
| `data_discovery` | Classificacao continua PII/PHI/PCI em transito; catalog por tenant; Shadow AI detection; alimenta `dlp` | **enterprise** |
| `dlp` | Data Loss Prevention: block/redact/alert/quarantine por tipo e tenant; audit trail; integra com `data_discovery` | **enterprise** |
| `finops` | Governanca de custos multi-tenant: budgets em 3 niveis (tenant/app/key), alertas webhook, export CSV | **enterprise** |

> **Ordem mandatoria do Security Pipeline enterprise:** `guardrails -> data_discovery -> dlp -> finops`. Ver [docs/specs/](docs/specs/) para specs completas (RF-32 a RF-35) e [docs/specs/RF-10-ADENDO-TENANT-CTX.md](docs/specs/RF-10-ADENDO-TENANT-CTX.md) para o mecanismo de contexto multi-tenant.

Exemplo de pipeline em `config.json`:

```json
"plugins": {
  "enabled": true,
  "pipeline": [
    { "name": "logging", "enabled": true },
    { "name": "pii_redactor", "enabled": true, "config": { "patterns": ["cpf", "email"], "log_redactions": true } },
    { "name": "semantic_cache", "enabled": true, "config": { "threshold": 0.95 } }
  ]
}
```

### Executar

```bash
./hermes     # usa config.json do diretorio atual
```

## Testes

### Testes Funcionais

```bash
# Gateway deve estar rodando
bash scripts/test/test_gateway.sh http://localhost:8080
```

Executa 12 testes: health check, request IDs, modelos, chat completions, embeddings, JSON mode, validacoes.

### Stress Test

```bash
# 256 requests, 24 concorrentes
bash scripts/stress/stress_test.sh http://localhost:8080 256 24

# Parametros: URL REQUESTS CONCURRENCY MODEL
bash scripts/stress/stress_test.sh http://localhost:8080 128 24 gemma3:1b
```

Mede throughput, taxa de erros e crescimento de memoria. Serve tambem para validar o build edge (`make edge`) sob carga.

### Benchmark Sincrono (Feature 11)

```bash
# Full (9 categorias: gerais + Python + Agent Coder)
bash scripts/benchmark/benchmark_sync.sh http://localhost:8080 _ full

# Basic (3 categorias: general_basic, python_basic, agent_basic)
bash scripts/benchmark/benchmark_sync.sh http://localhost:8080 _ basic

# Por grupo: general | python | agent
bash scripts/benchmark/benchmark_sync.sh http://localhost:8080 _ general

# Por categoria: general_basic | general_medium | general_advanced |
#                python_basic | python_medium | python_advanced |
#                agent_basic | agent_medium | agent_advanced
bash scripts/benchmark/benchmark_sync.sh http://localhost:8080 _ general_basic

# Com API key
bash scripts/benchmark/benchmark_sync.sh http://localhost:8080 your-api-key general_basic
```

Modelos: gemma3:1b, gemma3:4b, gemma3:12b (gerais/Python). Agent Coder: 3 modelos com tools (auto-detectados). Metricas: latency_ms, tokens, tokens_per_second, score. Saida: .json, .md, .csv.

## Fluxo de uma request (chat)

```mermaid
sequenceDiagram
    participant C as Cliente
    participant G as Gateway
    participant A as IAuth
    participant R as IRateLimiter
    participant P as Pipeline
    participant Cache as ICache
    participant Q as IRequestQueue
    participant O as Ollama

    C->>G: POST /v1/chat/completions
    G->>A: validate(token)
    A-->>G: ok / 401
    G->>R: allow(id)
    R-->>G: ok / 429
    G->>P: run_before_request
    P-->>G: PluginResult
    alt cached_response
        G-->>C: 200 + response
    else continue
        G->>Cache: get(key)
        alt hit
            G-->>C: 200 + cached
        else miss
            G->>Q: enqueue ou exec direto
            Q->>O: chat completion
            O-->>Q: response
            G->>P: run_after_response
            G->>Cache: put(key, response)
            G-->>C: 200 + response
        end
    end
```

## Documentacao

- **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** — Visao geral da arquitetura, core vs plugins, pipeline e build edge com diagramas Mermaid.
- **Especificacoes** — [docs/spec/](docs/spec/) contem 30 specs RF-XX (FUNCIONAL, UX, AI) + 1 ADR.
- **[docs/COMPARISON_HERMES_VS_ALLTRUE.md](docs/COMPARISON_HERMES_VS_ALLTRUE.md)** — Comparacao do HERMES com a plataforma AllTrue.ai (Varonis) por pilares AI TRiSM.

## Estrutura do Projeto

```mermaid
flowchart TB
    subgraph src [src/]
        main[main.cpp]
        gateway[gateway.h/cpp]
        ollama[ollama_client]
        config[config.h]
        plugin[plugin.h]
        core_services[core_services.h]
        metrics_if[metrics_interface.h]
        metrics[metrics.h]
        plugins_dir[plugins/*.h/cpp]
    end
    subgraph scripts [scripts/]
        benchmark[benchmark/]
        stress[stress/]
        test[test/]
    end
    subgraph docs [docs/spec/]
        specs[36 specs RF-XX]
    end
    main --> gateway
    gateway --> plugin
    gateway --> core_services
    gateway --> metrics_if
```

| Caminho | Descricao |
|---------|-----------|
| `src/main.cpp` | Ponto de entrada e signal handling |
| `src/gateway.h` / `.cpp` | Servidor HTTP, roteamento, handlers |
| `src/ollama_client.h` / `.cpp` | Cliente Ollama com balanceamento e retry |
| `src/core_services.h` | Interfaces ICache, IAuth, IRateLimiter, IRequestQueue + null impl |
| `src/plugin.h` | Plugin, PluginManager, pipeline |
| `src/metrics_interface.h` | IMetricsCollector; full em metrics.h, minimal em metrics_minimal |
| `src/plugins/*` | Plugins builtin (cache, auth, logging, pii_redactor, etc.) |
| `config.json` | Configuracao default |
| `scripts/benchmark/` | Benchmark sincrono |
| `scripts/stress/` | Testes de carga |
| `scripts/test/` | Testes funcionais |
| `docs/spec/` | Especificacoes RF-XX (30 FUNCIONAL + 4 AI + 1 UX + 1 ADR) |

## CI/CD

```mermaid
flowchart LR
    subgraph jobs [GitHub Actions]
        J1[build]
        J2[asan]
        J3[tsan]
        J4[edge]
        J5[docker]
        J6[docker-edge]
    end
    J1 -->|smoke tests| OK1[OK]
    J2 -->|AddressSanitizer| OK2[OK]
    J3 -->|ThreadSanitizer| OK3[OK]
    J4 -->|make edge + health| OK4[OK]
    J5 -->|imagem + container| OK5[OK]
    J6 -->|imagem edge + health| OK6[OK]
```

1. **build** — Compilacao com flags de producao + smoke tests
2. **asan** — Build com AddressSanitizer + testes
3. **tsan** — Build com ThreadSanitizer + testes
4. **edge** — Build minimal (`make edge`) + smoke test (health, metrics)
5. **docker** — Build da imagem Docker + teste do container
6. **docker-edge** — Build da imagem Docker edge (`Dockerfile.edge`) + smoke test

## Stack Tecnica

- **Linguagem**: C++23
- **HTTP Server/Client**: [cpp-httplib](https://github.com/yhirose/cpp-httplib) v0.34.0 (header-only)
- **JSON**: [jsoncpp](https://github.com/open-source-parsers/jsoncpp)
- **Build**: g++ com LTO, `-O3`, `-march=x86-64-v2`
- **Container**: Debian 12-slim (multi-stage)
- **Criptografia**: SHA-256 implementacao propria (sem dependencias externas)

## Roadmap

Especificacoes detalhadas em `docs/spec/` no formato RF-XX:

**Nao implementadas (RF com status RASCUNHO):**
- RF-02: Usage Tracking e Quotas
- RF-05: Prompt Management
- RF-06: Dashboard Web
- RF-08: A/B Testing
- RF-09: Webhook Notifications

**Parcialmente implementadas:**
- RF-16: Response Validator
- RF-19: Conversation Memory
- RF-20: Cost Controller
- RF-21: Adaptive Rate Limiter

**Stubs:**
- RF-23: Streaming Transformer
- RF-26: Model Warmup

## Licenca

[A definir]
