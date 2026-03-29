# RF-31 — Log Ingestion & Query (REST + gRPC)

- **RF:** RF-31
- **Titulo:** Log Ingestion & Query
- **Autor:** HERMES Team
- **Data:** 2026-03-17
- **Versao:** 1.0
- **Status:** RASCUNHO

## Objetivo

Expor endpoints REST e gRPC para **ingestao** e **consulta** de logs estruturados no HERMES. A feature permite que servicos externos (aplicacoes, sidecars, agentes de observabilidade) enviem logs para o gateway e que operadores consultem, filtrem e exportem esses logs via API. O modelo de dados segue a especificacao OpenTelemetry Logs para garantir interoperabilidade com coletores OTel, Grafana Loki, Datadog e outros backends.

Diferente do Audit Log (RF-04) — que registra automaticamente cada request/response do gateway — o RF-31 e um **servico de logs de proposito geral** para ingestao e consulta de registros arbitrarios enviados por clientes externos ou pelo proprio gateway.

## Escopo

- **Inclui:**
  - Endpoint REST para ingestao em batch (`POST /v1/logs`)
  - Endpoint REST para consulta com filtros (`GET /v1/logs`)
  - Endpoint REST para obter log individual (`GET /v1/logs/{id}`)
  - Servico gRPC `LogService` com RPCs `Ingest`, `Query` e `Tail`
  - Definicao protobuf em `proto/logs/v1/log_service.proto`
  - Modelo de dados alinhado ao OpenTelemetry (Resource, ScopeLogs, LogRecord)
  - Persistencia em JSONL com rotacao (reutiliza mecanismo do RF-04)
  - Autenticacao via API key (Bearer) para REST e metadata para gRPC

- **Nao inclui:**
  - Full-text search com indice invertido (busca e linear em JSONL)
  - Storage em banco de dados (ClickHouse, ElasticSearch)
  - Forwarding para backends externos (Loki, Datadog)
  - Alertas baseados em logs (coberto por RF-09 Webhooks)
  - Metricas derivadas de logs

## Descricao Funcional Detalhada

### Modelo de Dados

O modelo segue a estrutura OpenTelemetry, simplificada para o contexto do HERMES:

```
ResourceLogs
├── resource: Resource (service.name, host.name, ...)
└── scope_logs[]
    ├── scope: InstrumentationScope (name, version)
    └── log_records[]
        ├── id: string (UUID, gerado pelo servidor se ausente)
        ├── timestamp: int64 (Unix epoch nanoseconds)
        ├── observed_timestamp: int64 (quando o gateway recebeu)
        ├── severity_number: SeverityNumber (1-24, OTel enum)
        ├── severity_text: string ("INFO", "ERROR", etc.)
        ├── body: string | JSON (conteudo do log)
        ├── attributes: map<string, AnyValue> (metadata estruturada)
        ├── trace_id: bytes (16 bytes, correlacao com traces)
        ├── span_id: bytes (8 bytes, correlacao com spans)
        └── flags: uint32 (OTel log record flags)
```

#### SeverityNumber (OTel-compatible)

| Range | Nome |
|-------|------|
| 1-4 | TRACE |
| 5-8 | DEBUG |
| 9-12 | INFO |
| 13-16 | WARN |
| 17-20 | ERROR |
| 21-24 | FATAL |

### Fluxo de Ingestao

```
Cliente (REST/gRPC) → Auth → Validacao → Normalizacao → Buffer async → Worker → JSONL
                                              ↓
                                 observed_timestamp + id (se ausente)
```

1. Cliente envia batch de `ResourceLogs` via REST ou gRPC.
2. Gateway valida auth (API key).
3. Gateway valida schema (campos obrigatorios: timestamp, severity_number, body).
4. Gateway enriquece: `observed_timestamp` (agora), `id` (UUID se ausente).
5. Gateway enfileira em buffer async (nao bloqueia o caller).
6. Worker persiste em JSONL com rotacao por dia e tamanho.
7. Resposta parcial: aceito com contagem de registros ingeridos.

### Fluxo de Consulta

```
Cliente (REST/gRPC) → Auth → Parse filtros → Leitura JSONL → Filtragem → Resposta paginada
```

1. Cliente envia query com filtros (service, severity, time range, texto, atributos).
2. Gateway valida auth.
3. Gateway le arquivos JSONL no range de datas solicitado.
4. Aplica filtros em memoria (busca linear).
5. Retorna resultados paginados (limit/offset ou cursor).

### Fluxo de Tail (gRPC only)

```
Cliente gRPC → Auth → Subscribe → Server-stream de novos logs em tempo real
```

1. Cliente abre stream `Tail` com filtros opcionais.
2. Gateway envia logs novos conforme sao ingeridos (fan-out do buffer).
3. Stream permanece aberto ate o cliente cancelar ou timeout.

---

## Contrato REST (OpenAPI 3.0.3)

### POST /v1/logs — Ingestao em batch

**Request Body:**

```json
{
  "resource_logs": [
    {
      "resource": {
        "attributes": {
          "service.name": "my-app",
          "host.name": "node-01"
        }
      },
      "scope_logs": [
        {
          "scope": {
            "name": "my-app.auth",
            "version": "1.0.0"
          },
          "log_records": [
            {
              "timestamp": 1742198400000000000,
              "severity_number": 9,
              "severity_text": "INFO",
              "body": "User login successful",
              "attributes": {
                "user.id": "u-123",
                "action": "login"
              },
              "trace_id": "0af7651916cd43dd8448eb211c80319c",
              "span_id": "b7ad6b7169203331"
            }
          ]
        }
      ]
    }
  ]
}
```

**Response 202 (Accepted):**

```json
{
  "accepted": 1,
  "rejected": 0,
  "errors": []
}
```

**Response 207 (Partial Success):**

```json
{
  "accepted": 8,
  "rejected": 2,
  "errors": [
    { "index": 3, "message": "missing required field: severity_number" },
    { "index": 7, "message": "timestamp out of valid range" }
  ]
}
```

**Response 400:**

```json
{
  "error": {
    "code": "INVALID_REQUEST",
    "message": "resource_logs is required and must be a non-empty array"
  }
}
```

### GET /v1/logs — Consulta com filtros

**Query Parameters:**

| Parametro | Tipo | Obrigatorio | Descricao |
|-----------|------|-------------|-----------|
| `service` | string | nao | Filtrar por `service.name` |
| `severity_min` | int | nao | Severidade minima (1-24) |
| `severity_max` | int | nao | Severidade maxima (1-24) |
| `from` | int64 | nao | Timestamp inicio (epoch ns) |
| `to` | int64 | nao | Timestamp fim (epoch ns) |
| `body_contains` | string | nao | Substring no body |
| `trace_id` | string | nao | Filtrar por trace_id |
| `attribute` | string | nao | Filtro `key=value` (repetivel) |
| `order` | string | nao | `asc` ou `desc` (default: `desc`) |
| `limit` | int | nao | Max resultados (default: 100, max: 1000) |
| `cursor` | string | nao | Token de paginacao (opaco, base64) |

**Response 200:**

```json
{
  "log_records": [
    {
      "id": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
      "timestamp": 1742198400000000000,
      "observed_timestamp": 1742198400050000000,
      "severity_number": 9,
      "severity_text": "INFO",
      "body": "User login successful",
      "resource": {
        "service.name": "my-app",
        "host.name": "node-01"
      },
      "scope": {
        "name": "my-app.auth",
        "version": "1.0.0"
      },
      "attributes": {
        "user.id": "u-123",
        "action": "login"
      },
      "trace_id": "0af7651916cd43dd8448eb211c80319c",
      "span_id": "b7ad6b7169203331"
    }
  ],
  "total": 4230,
  "next_cursor": "eyJ0cyI6MTc0MjE5ODQwMH0=",
  "has_more": true
}
```

### GET /v1/logs/{id} — Log individual

**Response 200:**

```json
{
  "id": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
  "timestamp": 1742198400000000000,
  "observed_timestamp": 1742198400050000000,
  "severity_number": 9,
  "severity_text": "INFO",
  "body": "User login successful",
  "resource": {
    "service.name": "my-app",
    "host.name": "node-01"
  },
  "scope": {
    "name": "my-app.auth",
    "version": "1.0.0"
  },
  "attributes": {
    "user.id": "u-123",
    "action": "login"
  },
  "trace_id": "0af7651916cd43dd8448eb211c80319c",
  "span_id": "b7ad6b7169203331"
}
```

**Response 404:**

```json
{
  "error": {
    "code": "NOT_FOUND",
    "message": "Log record not found"
  }
}
```

### Autenticacao

Todos os endpoints requerem header `Authorization: Bearer <API_KEY>` ou `X-API-Key: <API_KEY>`.

### Headers de Response

| Header | Descricao |
|--------|-----------|
| `X-Request-Id` | UUID da requisicao |
| `X-Accepted-Count` | Registros aceitos (POST) |
| `X-Rejected-Count` | Registros rejeitados (POST) |

---

## Contrato gRPC

Definicao completa em `proto/logs/v1/log_service.proto`.

### Servico

```protobuf
service LogService {
  rpc Ingest(IngestLogsRequest) returns (IngestLogsResponse);
  rpc Query(QueryLogsRequest) returns (QueryLogsResponse);
  rpc Tail(TailLogsRequest) returns (stream LogRecord);
}
```

### RPCs

| RPC | Tipo | Descricao |
|-----|------|-----------|
| `Ingest` | Unary | Ingestao em batch de ResourceLogs |
| `Query` | Unary | Consulta com filtros e paginacao |
| `Tail` | Server-streaming | Stream de logs novos em tempo real |

### Autenticacao gRPC

Via metadata: `authorization: Bearer <API_KEY>`.

---

## Configuracao

### config.json

```json
{
  "logs": {
    "enabled": true,
    "storage": {
      "type": "file",
      "directory": "logs/ingest/",
      "max_file_size_mb": 256,
      "retention_days": 14,
      "flush_interval_seconds": 2
    },
    "ingestion": {
      "max_batch_size": 10000,
      "max_body_size_bytes": 5242880,
      "buffer_size": 50000,
      "workers": 2
    },
    "query": {
      "default_limit": 100,
      "max_limit": 1000,
      "max_time_range_hours": 168
    },
    "grpc": {
      "enabled": true,
      "port": 9090,
      "max_message_size_mb": 10,
      "tail_buffer_size": 1000,
      "tail_max_duration_seconds": 3600
    }
  }
}
```

### Variaveis de Ambiente

| Variavel | Descricao | Default |
|----------|-----------|---------|
| `LOGS_ENABLED` | Habilitar log ingestion | `false` |
| `LOGS_DIR` | Diretorio de armazenamento | `logs/ingest/` |
| `LOGS_RETENTION_DAYS` | Dias de retencao | `14` |
| `LOGS_GRPC_PORT` | Porta gRPC | `9090` |
| `LOGS_GRPC_ENABLED` | Habilitar endpoint gRPC | `false` |
| `LOGS_MAX_BATCH_SIZE` | Max registros por batch | `10000` |

## Endpoints

| Protocolo | Metodo/RPC | Path/Service | Auth | Descricao |
|-----------|------------|--------------|------|-----------|
| REST | POST | `/v1/logs` | API Key | Ingestao em batch |
| REST | GET | `/v1/logs` | API Key | Consulta com filtros |
| REST | GET | `/v1/logs/{id}` | API Key | Log individual |
| gRPC | Ingest | `hermes.logs.v1.LogService` | metadata | Ingestao em batch |
| gRPC | Query | `hermes.logs.v1.LogService` | metadata | Consulta com filtros |
| gRPC | Tail | `hermes.logs.v1.LogService` | metadata | Stream em tempo real |

## Regras de Negocio

1. **Ingestao nao-bloqueante:** POST /v1/logs e Ingest gRPC enfileiram e retornam imediatamente (202 Accepted).
2. **Validacao de campos obrigatorios:** `timestamp`, `severity_number` e `body` sao obrigatorios em cada LogRecord. Registros invalidos sao rejeitados individualmente (partial success).
3. **Limite de batch:** Maximo de `max_batch_size` registros por chamada; acima disso, 413 Payload Too Large.
4. **Enriquecimento automatico:** `observed_timestamp` e `id` sao preenchidos pelo servidor se ausentes.
5. **Rotacao de arquivos:** Por dia (logs-YYYY-MM-DD.jsonl) e por tamanho (`max_file_size_mb`). Retencao conforme `retention_days`.
6. **Consulta limitada por range:** `max_time_range_hours` impede scans em janelas excessivas.
7. **Tail (gRPC):** Fan-out do buffer de ingestao; cada subscriber recebe copia dos novos logs. Max duracao `tail_max_duration_seconds`.
8. **Concorrencia:** Ingestao thread-safe via fila com backpressure. Consulta usa `shared_mutex` para nao bloquear escrita.
9. **Sem PII em body por default:** O pipeline de PII Redaction (RF-14) pode ser acionado para logs ingeridos se habilitado.
10. **Compatibilidade OTel:** O modelo de dados e compatible com o OTLP Logs protocol, permitindo que coletores OTel enviem logs diretamente ao HERMES.

## Limites e Excecoes

| Cenario | Comportamento |
|---------|--------------|
| Batch > max_batch_size | 413 Payload Too Large |
| Body > max_body_size_bytes | 413 Payload Too Large |
| Campo obrigatorio ausente | Registro rejeitado (partial success 207) |
| Buffer cheio (backpressure) | 503 Service Unavailable (retry com backoff) |
| Time range > max_time_range_hours | 400 Bad Request |
| Log nao encontrado (GET by id) | 404 Not Found |
| Auth invalida | 401 Unauthorized |
| Tail sem novos logs | Heartbeat vazio a cada 30s para keepalive |

## Dependencias e Integracoes

- **Internas:** Logger, PluginManager (pipeline PII opcional), API Keys (auth), filesystem
- **Externas:** gRPC C++ runtime (grpc++), protobuf
- **Pre-requisito:** Feature 10 (Plugin System) para integracao na pipeline
- **Relacionados:** RF-04 (Audit Log — modelo de persistencia JSONL reutilizado), RF-14 (PII Redaction — opcional para logs ingeridos)

## Criterios de Aceitacao

- [ ] POST /v1/logs aceita batch de ResourceLogs e retorna 202 com contagem
- [ ] Partial success (207) quando parte dos registros e invalida
- [ ] GET /v1/logs retorna logs com filtros (service, severity, time range, body_contains, trace_id, attribute)
- [ ] GET /v1/logs/{id} retorna log individual ou 404
- [ ] Paginacao por cursor funcional
- [ ] gRPC Ingest aceita batch e retorna contagem
- [ ] gRPC Query retorna logs filtrados com paginacao
- [ ] gRPC Tail envia logs novos em tempo real via server-stream
- [ ] Persistencia em JSONL com rotacao por dia e tamanho
- [ ] Retencao automatica por retention_days
- [ ] Auth obrigatoria em todos os endpoints (REST e gRPC)
- [ ] Buffer async nao bloqueia caller
- [ ] Backpressure retorna 503 quando buffer cheio
- [ ] Heartbeat em Tail a cada 30s quando sem novos logs

## Riscos e Trade-offs

1. **Busca linear em JSONL:** Para volumes altos (>1M registros/dia), a consulta sera lenta. Mitigacao futura: indice por timestamp + service.name ou integracao com ClickHouse.
2. **gRPC como nova dependencia:** Adiciona grpc++ e protobuf ao build. Impacta tamanho do binario e complexidade do Makefile. Build edge deve excluir gRPC.
3. **Concorrencia de escrita:** Multiplos workers escrevendo em JSONL requer coordenacao. Usar single-writer com fila.
4. **Tail fan-out:** Muitos subscribers de Tail podem consumir memoria. Limitar por configuracao.
5. **Tamanho de body:** Logs com body grande (prompts LLM completos) podem crescer rapido em disco. Comprimir opcionalmente (gzip por arquivo).
6. **Compatibilidade OTel parcial:** Implementamos o modelo de dados OTel mas nao o protocolo OTLP completo (HTTP/protobuf). Coletores OTel precisarao de exporter customizado ou adapter.

## Status de Implementacao

NAO IMPLEMENTADO — Especificacao de contrato. Aguardando revisao e aprovacao para desenvolvimento.

## Checklist de Qualidade

- [x] Objetivo claro e testavel
- [x] Escopo dentro/fora definido
- [x] Regras de negocio sem ambiguidade
- [x] Criterios de aceitacao verificaveis
- [x] Excecoes e limites cobertos
