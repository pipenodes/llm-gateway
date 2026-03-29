# RF-21 — Adaptive Rate Limiter

- **RF:** RF-21
- **Titulo:** Adaptive Rate Limiter
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Plugin de rate limiting inteligente que se adapta a saude dos backends em tempo real. Quando a latencia sobe ou erros aumentam, o rate limit e reduzido automaticamente para proteger os backends. Inclui circuit breaker pattern para parar de enviar requests a backends que estao falhando repetidamente.

## Escopo

- **Inclui:** Rate limit adaptativo por saude do backend; circuit breaker (Closed/Open/HalfOpen); health monitor com metricas (latencia p95, taxa de erro, requests ativas); recuperacao gradual; suporte multi-backend; integracao com `/metrics`.
- **Nao inclui:** Rate limiting estatico sem adaptacao; integracao com IRateLimiter real (rpm_by_key configurado mas nao integrado).

## Descricao Funcional Detalhada

### Fluxo

1. Request chega ao Monitor de Saude.
2. Se backend saudavel: rate limit normal.
3. Se backend degradado: rate limit reduzido.
4. Se backend falho: circuit breaker OPEN, bloqueia tudo.
5. Apos timeout: circuit HALF-OPEN, permite 1 request de teste.
6. Se sucesso: recuperacao gradual (5 -> 10 -> 30 -> 60).
7. Se falha: volta para OPEN.

### Metricas Monitoradas

- Latencia p95
- Taxa de erro
- Requests ativas (concurrency)

### Cenarios

- Backend sobrecarregado: reduz rate de 60 para 5 req/min ate normalizar.
- Circuit breaker: backend cai, para 30s, health check, retoma se saudavel.
- Recuperacao gradual: aumenta progressivamente em vez de liberar tudo.
- Multi-backend: cada backend tem estado proprio; um lento nao afeta os outros.

## Interface / Contrato

```cpp
enum class CircuitState {
    Closed,    // Normal -- tudo passa
    Open,      // Aberto -- bloqueia tudo
    HalfOpen   // Semi-aberto -- permite 1 request de teste
};

struct BackendHealth {
    CircuitState state = CircuitState::Closed;
    double current_rate_multiplier = 1.0;  // 1.0 = 100% do rate limit
    double avg_latency_ms = 0;
    double p95_latency_ms = 0;
    double error_rate = 0;
    int consecutive_failures = 0;
    int64_t last_failure_at = 0;
    int64_t circuit_open_until = 0;
};

class AdaptiveRateLimiterPlugin : public Plugin {
    PluginResult before_request(Json::Value& body, RequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, RequestContext& ctx) override;
};
```

## Configuracao

```json
{
  "plugins": {
    "pipeline": [
      {
        "name": "adaptive_rate_limiter",
        "enabled": true,
        "config": {
          "latency_warning_ms": 5000,
          "latency_critical_ms": 15000,
          "error_rate_warning": 0.1,
          "error_rate_critical": 0.5,
          "circuit_open_seconds": 30,
          "circuit_failure_threshold": 5,
          "recovery_step": 0.25,
          "window_seconds": 60
        }
      }
    ]
  }
}
```

## Endpoints

Metricas em `/metrics`:

```json
{
  "adaptive_rate_limiter": {
    "backends": {
      "localhost:11434": {
        "state": "closed",
        "rate_multiplier": 1.0,
        "avg_latency_ms": 1200,
        "p95_latency_ms": 3400,
        "error_rate": 0.02,
        "consecutive_failures": 0
      }
    }
  }
}
```

## Regras de Negocio

1. Cold start: sem amostras suficientes, comecar com rate_multiplier=1.0.
2. Circuit OPEN apos N falhas consecutivas (default 5).
3. Circuit HALF-OPEN apos timeout (default 30s).
4. Recuperacao gradual: recovery_step (default 0.25) por intervalo.
5. Cada backend tem estado independente.
6. Sliding window de 60s para metricas.

## Dependencias e Integracoes

- **Internas:** Feature 10 (Plugin System), OllamaClient (metricas de latencia).
- **Externas:** Nenhuma.

## Criterios de Aceitacao

- [ ] Plugin reduz rate limit quando latencia p95 > latency_warning_ms
- [ ] Circuit breaker abre apos circuit_failure_threshold falhas consecutivas
- [ ] Circuit HALF-OPEN permite 1 request de teste apos circuit_open_seconds
- [ ] Recuperacao gradual aumenta rate_multiplier por recovery_step
- [ ] Metricas expostas em /metrics por backend
- [ ] Integracao com IRateLimiter real (pendente)

## Riscos e Trade-offs

1. **Sensibilidade:** Thresholds muito sensiveis causam oscilacao (flapping). Precisa de histerese.
2. **Latencia do health check:** Health check no half-open precisa ser rapido.
3. **Interacao com rate limiter existente:** Coordenar com RateLimiter existente.
4. **Streaming:** Latencia de streaming diferente de non-streaming; considerar metricas separadas.

## Status de Implementacao

IMPLEMENTADO — AdaptiveRateLimiterPlugin reescrito com: circuit breaker (Closed/Open/HalfOpen), health monitoring via sliding window (latencia p95, error rate), rate_multiplier adaptativo, recuperacao gradual, IAuditSink integration, metricas detalhadas por backend em stats().

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
