# RF-27 — Alerting

- **RF:** RF-27
- **Titulo:** Alerting
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Plugin que implementa IAuditSink para receber eventos "request completed" e avalia regras configuraveis (ex.: N respostas 4xx por key em janela de tempo; bloqueio por prompt injection). Ao disparar, envia notificacao via webhook (HTTP POST). Thread-safe e nao bloqueante no path da request (enfileirar envio; worker envia).

## Escopo

- **Inclui:** IAuditSink; regras status_4xx, blocked, latency; webhook HTTP POST; worker assincrono; payload JSON com event, timestamp, entries, rule.
- **Nao inclui:** Regras customizadas via DSL; integracao com Slack/Teams nativa (via webhook generico).

## Descricao Funcional Detalhada

### Concorrencia

- on_request_completed(entry) chamado de multiplas threads.
- Atualiza contadores/estado sob mutex (leve).
- Enfileira payload de alerta para envio.
- Retorna imediatamente.
- Worker thread envia HTTP POST; nao bloqueia thread da request.

### Regras (exemplos)

- **status_4xx:** Contagem de respostas 4xx por key (ou global) em janela; se ultrapassar limite, dispara.
- **blocked:** Qualquer request bloqueada (400 prompt injection, 402 cost) pode gerar alerta.
- **latency:** Latencia acima de percentil (p99 > X ms) em janela — opcional.

## Interface / Contrato

- Interface: IAuditSink
- Metodo: on_request_completed(AuditEntry)
- Config: lista de regras com tipo, parametros (window_seconds, limit, key_alias opcional), webhook_url por regra ou global.

## Configuracao

```json
{
  "alerting": {
    "enabled": true,
    "webhook_url": "https://example.com/webhook",
    "rules": [
      { "type": "status_4xx", "window_seconds": 300, "limit": 10 },
      { "type": "blocked", "any": true }
    ]
  }
}
```

## Endpoints

N/A — plugin que envia para webhook configurado.

### Payload do Webhook

POST com Content-Type: application/json. Corpo JSON:

- `event`: tipo da regra
- `timestamp`: ISO8601
- `entries`: array de AuditEntry que dispararam
- `rule`: nome/params da regra

## Regras de Negocio

1. Thread-safe: mutex leve para contadores.
2. Nao bloqueante: enfileirar e worker envia.
3. Regras avaliadas apos cada request completed.
4. Webhook URL obrigatorio quando enabled.

## Dependencias e Integracoes

- **Internas:** Feature 04 (core IAuditSink), Feature 10 (Plugin System).
- **Externas:** Endpoint webhook do cliente.

## Criterios de Aceitacao

- [ ] IAuditSink implementado
- [ ] Regra status_4xx dispara ao ultrapassar limit em window
- [ ] Regra blocked dispara em qualquer bloqueio
- [ ] Webhook HTTP POST com payload JSON
- [ ] Worker assincrono nao bloqueia request path
- [ ] Thread-safe

## Riscos e Trade-offs

1. **Falha do webhook:** Retry ou dead letter; nao bloquear audit path.
2. **Volume:** Muitos alertas podem sobrecarregar webhook; considerar rate limit.
3. **Latencia:** Regras complexas (latency p99) podem exigir mais estado.

## Status de Implementacao

IMPLEMENTADO — IAuditSink, webhook HTTP POST, regras por status/blocked, worker assincrono.

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
