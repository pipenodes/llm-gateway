# RF-09 — Webhook Notifications

- **RF:** RF-09
- **Titulo:** Webhook Notifications
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Sistema de notificacoes via webhook que envia eventos do gateway para URLs externas configuradas. Permite integrar com Slack, Discord, PagerDuty, Microsoft Teams ou qualquer endpoint HTTP para alertas em tempo real sobre eventos criticos (backend fora do ar, rate limit atingido, key comprometida, etc.).

## Escopo

- **Inclui:** WebhookDispatcher, WebhookEndpoint, WebhookEvent, fila assincrona, retry com backoff, filtro por tipo de evento, endpoints admin
- **Nao inclui:** Dead letter queue, adapters por plataforma (Slack/PagerDuty format), deduplicacao ou rate limiting de eventos

## Descricao Funcional Detalhada

### Arquitetura

Evento no Gateway -> WebhookDispatcher -> Filtro por Tipo -> Buffer Async -> Worker Thread -> POST JSON para cada endpoint configurado. Em caso de falha: Retry com Backoff -> Worker.

### Componentes

- **WebhookDispatcher:** Recebe eventos e despacha para os endpoints configurados. Gerencia fila assincrona e worker thread.
- **WebhookEndpoint:** Configuracao de um destino (URL, event_types, headers, max_retries, timeout_ms).
- **WebhookEvent:** Estrutura do payload (type, type_name, timestamp, data).

### Tipos de Evento

BackendDown, BackendUp, RateLimitHit, QuotaWarning, QuotaExceeded, KeyCreated, KeyRevoked, AuthFailure, HighLatency, ErrorSpike, Custom.

## Interface / Contrato

```cpp
enum class EventType {
    BackendDown, BackendUp, RateLimitHit, QuotaWarning, QuotaExceeded,
    KeyCreated, KeyRevoked, AuthFailure, HighLatency, ErrorSpike, Custom
};

struct WebhookEvent {
    EventType type;
    std::string type_name;
    int64_t timestamp;
    Json::Value data;
};

struct WebhookEndpoint {
    std::string name;
    std::string url;
    std::vector<EventType> event_types;
    std::unordered_map<std::string, std::string> headers;
    int max_retries = 3;
    int timeout_ms = 5000;
    bool enabled = true;
};

class WebhookDispatcher {
public:
    void init(const std::vector<WebhookEndpoint>& endpoints);
    void emit(EventType type, const Json::Value& data);
    void shutdown();
    [[nodiscard]] Json::Value stats() const;
private:
    void worker_loop();
    bool send(const WebhookEndpoint& endpoint, const WebhookEvent& event);
};
```

## Configuracao

### config.json

```json
{
  "webhooks": {
    "enabled": true,
    "endpoints": [
      {
        "name": "slack-alerts",
        "url": "https://hooks.slack.com/services/XXX/YYY/ZZZ",
        "event_types": ["backend_down", "backend_up", "error_spike"],
        "max_retries": 3,
        "timeout_ms": 5000
      },
      {
        "name": "audit-channel",
        "url": "https://hooks.slack.com/services/AAA/BBB/CCC",
        "event_types": ["key_created", "key_revoked", "auth_failure"]
      },
      {
        "name": "pagerduty",
        "url": "https://events.pagerduty.com/v2/enqueue",
        "headers": { "Content-Type": "application/json" },
        "event_types": ["backend_down", "quota_exceeded"]
      }
    ]
  }
}
```

### Variaveis de Ambiente

| Variavel | Descricao | Default |
|----------|-----------|---------|
| `WEBHOOKS_ENABLED` | Habilitar webhooks | `false` |
| `WEBHOOK_URL` | URL unica de webhook (atalho para config simples) | (vazio) |

## Endpoints

| Metodo | Path | Descricao |
|--------|------|-----------|
| `GET` | `/admin/webhooks` | Listar endpoints e metricas |
| `POST` | `/admin/webhooks/test` | Enviar evento de teste |

### Payload enviado ao webhook (exemplo backend_down)

```json
{
  "event": "backend_down",
  "timestamp": 1740355200,
  "gateway": "hermes",
  "data": {
    "backend": "localhost:11434",
    "last_error": "connection refused",
    "consecutive_failures": 5
  }
}
```

## Regras de Negocio

- Envio assincrono: nao bloqueia o request handler.
- Retry com backoff em caso de falha (max_retries configuravel).
- event_types vazio = enviar todos os eventos.
- URLs de webhook sao secrets; nao aparecer em logs; mascarar na saida de /admin/webhooks.
- HTTPS obrigatorio para Slack, PagerDuty (httplib com OpenSSL).

## Dependencias e Integracoes

- **Internas:** httplib (POST ao webhook), Logger
- **Externas:** Nenhuma
- **Pre-requisito:** httplib compilado com suporte OpenSSL para HTTPS

## Criterios de Aceitacao

- [ ] WebhookDispatcher emite eventos para endpoints configurados
- [ ] Envio assincrono via fila (nao bloqueia handler)
- [ ] Retry com backoff ate max_retries
- [ ] Filtro por event_types funciona
- [ ] Endpoint /admin/webhooks/test envia evento de teste
- [ ] URLs mascaradas em /admin/webhooks

## Riscos e Trade-offs

1. **HTTPS:** Webhooks externos requerem HTTPS. httplib precisa de OpenSSL.
2. **Latencia:** Envio nao deve bloquear; fila assincrona obrigatoria.
3. **Flood:** Muitos eventos em sequencia podem gerar flood. Considerar deduplicacao ou rate limiting (max 1 evento do mesmo tipo por minuto).
4. **Secrets:** URLs sao secrets; nao em logs; mascarar em admin.
5. **Falha do destino:** Apos max_retries, eventos sao perdidos. Considerar dead letter queue futura.
6. **Formato:** Cada plataforma tem formato diferente; pode precisar adapters (Slack, PagerDuty).

## Status de Implementacao

IMPLEMENTADO — WebhookPlugin implementado com: fila assincrona com worker thread, retry com backoff exponencial (1s, 2s, 4s...), filtro por event_types, suporte HTTP e HTTPS (httplib), headers customizaveis, URLs mascaradas em /admin/webhooks, endpoint POST /admin/webhooks/test, 11 tipos de evento.

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
