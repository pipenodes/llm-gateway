# RF-04 — Audit Log (Core + Plugin)

- **RF:** RF-04
- **Titulo:** Audit Log (Core + Plugin)
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Implementar um sistema de audit log que registra cada request/response processada pelo gateway em formato JSONL ou Syslog (RFC 5424). Permite rastreabilidade completa, analise forense, debugging e compliance. O **core** define a estrutura AuditEntry e a interface IAuditSink; o **plugin audit** implementa persistencia (JSONL, Syslog, HTTP, S3), rotacao e endpoint GET /admin/audit.

## Escopo

- **Inclui:** AuditEntry, IAuditSink, IAuditQueryProvider, plugin audit, persistencia JSONL/Syslog/HTTP/S3, rotacao, GET /admin/audit, GET /admin/audit/stats
- **Nao inclui:** Indice/SQLite para consultas em volumes altos, alertas baseados em audit

## Descricao Funcional Detalhada

### Core vs Plugin

- **Core:** Define AuditEntry e IAuditSink em core_services.h. Gateway chama PluginManager::notify_request_completed(entry) apos cada request (apos run_after_response). Nenhum sink registrado por defeito; implementacoes devem ser thread-safe e nao bloqueantes (preferir fila + worker no plugin).
- **Plugin audit:** Implementa IAuditSink e opcionalmente participa do pipeline. Responsavel por persistencia (JSONL, Syslog, HTTP, S3), rotacao e endpoint GET /admin/audit.

### Arquitetura

Request -> Handler -> Backend -> Response -> notify_request_completed -> IAuditSink(s) -> Plugin audit -> Fila thread-safe -> Worker -> audit-YYYY-MM-DD.jsonl / Syslog. GET /admin/audit -> Plugin audit -> Leitura de arquivos.

### Componentes (Core)

- **AuditEntry:** Estrutura em core_services.h que descreve um evento de audit.
- **IAuditSink:** Interface no core; metodo on_request_completed(entry). Plugins que implementam recebem o evento apos cada request.
- **PluginManager::notify_request_completed(entry):** Invocado pelo gateway apos run_after_response nos handlers de chat/completions/embeddings.

### Componentes (Plugin audit)

- Fila + worker: on_request_completed enfileira e retorna imediatamente (nao bloqueante).
- Persistencia: JSONL (arquivo local), Syslog (RFC 5424), HTTP, S3.
- IAuditQueryProvider: query(AuditQuery) para o gateway servir GET /admin/audit.

### Concorrencia

- on_request_completed chamado de multiplas threads; apenas enfileira (fila com limite, backpressure). Retorna imediatamente.
- Worker threads leem da fila e persistem. Escrita em arquivo sob lock ou thread unica.
- GET /admin/audit leitura a partir de ficheiros rotacionados ou buffer com shared_mutex; nao bloquear worker de escrita.

### Formatos de saida

- **JSONL:** Uma linha JSON por evento (campos de AuditEntry). Usado para arquivo local, HTTP ou S3.
- **Syslog (RFC 5424):** Mensagem IETF Syslog. Corpo pode ser JSON do evento para SIEM/colectores.

### Cenarios

1. **Debugging:** Cliente reporta resposta errada. Buscar request ID e ver exatamente o que foi enviado/recebido.
2. **Compliance:** Regulacoes exigem registro de todas as interacoes com IA.
3. **Analise de uso:** Patterns de uso — modelos mais usados, horarios de pico, tamanho medio de prompts.
4. **Deteccao de abuso:** Identificar keys com requests suspeitas.

## Interface / Contrato

```cpp
struct AuditEntry {
    int64_t timestamp;
    std::string request_id;
    std::string key_alias;
    std::string client_ip;
    std::string method;
    std::string path;
    std::string model;
    int status_code;
    int prompt_tokens;
    int completion_tokens;
    int64_t latency_ms;
    bool stream;
    bool cache_hit;
    std::string error;
};

struct AuditQuery {
    std::string key_alias;
    std::string model;
    int64_t from_timestamp = 0;
    int64_t to_timestamp = 0;
    int status_code = 0;
    int limit = 100;
    int offset = 0;
};

// Core
class IAuditSink {
public:
    virtual void on_request_completed(const AuditEntry& entry) = 0;
};

// Plugin implementa IAuditQueryProvider
class IAuditQueryProvider {
public:
    virtual Json::Value query(const AuditQuery& q) const = 0;
};
```

## Configuracao

### config.json

```json
{
  "audit": {
    "enabled": true,
    "output": "file",
    "format": "jsonl",
    "path": "audit/",
    "directory": "audit/",
    "max_file_size_mb": 100,
    "retention_days": 30,
    "flush_interval_seconds": 5,
    "include_body": false,
    "buffer_size": 1000,
    "queue_max_entries": 10000,
    "http_endpoint": null,
    "syslog_host": null,
    "syslog_port": 514,
    "syslog_protocol": "udp",
    "syslog_facility": 16,
    "syslog_severity": 6,
    "syslog_app_name": "hermes"
  }
}
```

- **output:** "file" | "http" | "s3" | "syslog"
- **format:** "jsonl" | "syslog"
- **queue_max_entries:** Limite da fila; quando cheia, politica (descartar antigos ou novos) conforme config.

### Variaveis de Ambiente

| Variavel | Descricao | Default |
|----------|-----------|---------|
| `AUDIT_ENABLED` | Habilitar audit log | `false` |
| `AUDIT_DIR` | Diretorio para arquivos de audit | `audit/` |
| `AUDIT_MAX_FILE_MB` | Tamanho maximo por arquivo | `100` |
| `AUDIT_RETENTION_DAYS` | Dias para manter logs | `30` |
| `AUDIT_INCLUDE_BODY` | Incluir body da request/response | `false` |

## Endpoints

| Metodo | Path | Descricao |
|--------|------|-----------|
| `GET` | `/admin/audit` | Consultar audit log com filtros |
| `GET` | `/admin/audit/stats` | Estatisticas agregadas |

### Query Parameters de `/admin/audit`

| Parametro | Tipo | Descricao |
|-----------|-----|-----------|
| `key` | string | Filtrar por alias da key |
| `model` | string | Filtrar por modelo |
| `from` | int64 | Timestamp inicial (epoch) |
| `to` | int64 | Timestamp final (epoch) |
| `status` | int | Filtrar por HTTP status |
| `limit` | int | Max resultados (default 100) |
| `offset` | int | Offset para paginacao |

### Response

```json
{
  "entries": [
    {
      "timestamp": 1740355200,
      "request_id": "abc123-def456",
      "key_alias": "dev-team",
      "client_ip": "192.168.1.10",
      "method": "POST",
      "path": "/v1/chat/completions",
      "model": "llama3:8b",
      "status_code": 200,
      "prompt_tokens": 150,
      "completion_tokens": 85,
      "latency_ms": 1240,
      "stream": false,
      "cache_hit": false
    }
  ],
  "total": 4230,
  "limit": 100,
  "offset": 0
}
```

### Formato JSONL (arquivo)

```
{"ts":1740355200,"rid":"abc123","key":"dev-team","ip":"192.168.1.10","method":"POST","path":"/v1/chat/completions","model":"llama3:8b","status":200,"pt":150,"ct":85,"lat_ms":1240,"stream":false,"cache":false}
```

## Regras de Negocio

- Implementacoes IAuditSink devem ser thread-safe e nao bloqueantes.
- Plugin audit: enfileira em on_request_completed e retorna imediatamente.
- Servidor Syslog: output "syslog" com syslog_host, syslog_port, syslog_protocol (udp|tcp|tls).
- Rotacao por dia/tamanho; retencao por retention_days.

## Dependencias e Integracoes

- **Internas:** Logger, crypto (timestamps), PluginManager
- **Externas:** Nenhuma
- **Sistema:** Filesystem para escrita de logs
- **Requer:** Feature 10 (Plugin/Middleware System) para plugin audit

## Criterios de Aceitacao

- [ ] Core define AuditEntry e IAuditSink
- [ ] Gateway chama notify_request_completed apos cada request
- [ ] Plugin audit implementa IAuditSink (fila + worker nao bloqueante)
- [ ] Persistencia JSONL em arquivo com rotacao
- [ ] Suporte a Syslog (UDP/TCP/TLS), HTTP, S3
- [ ] GET /admin/audit com filtros key, model, from, to, status, limit, offset
- [ ] GET /admin/audit/stats retorna estatisticas agregadas
- [ ] Requer ADMIN_KEY para endpoints admin

## Riscos e Trade-offs

1. **Performance de I/O:** Flush sincrono impactaria latencia. Usar buffer + flush assincrono.
2. **Espaco em disco:** Logs podem crescer rapidamente. Rotacao e retencao essenciais.
3. **Consulta em arquivos grandes:** Busca em JSONL e linear (O(n)). Para volumes altos, considerar indice ou SQLite.
4. **Dados sensiveis:** include_body=true armazena prompts em disco. Cuidado com LGPD/GDPR.
5. **Concorrencia de escrita:** Mutex necessario mas rapido (append only).
6. **Volume em /app/data:** Docker volume precisa de espaco suficiente.

## Status de Implementacao

IMPLEMENTADO — Core com AuditEntry, IAuditSink, notify_request_completed. Plugin audit com persistencia JSONL, fila+worker, GET /admin/audit. Suporte a Syslog/HTTP/S3 conforme config.

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
