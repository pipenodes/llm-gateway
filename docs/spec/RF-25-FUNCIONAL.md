# RF-25 — Request Deduplication

- **RF:** RF-25
- **Titulo:** Request Deduplication
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Plugin que detecta requests duplicadas em uma janela de tempo e compartilha a resposta entre elas. Se uma request identica ja esta em andamento, as requests subsequentes esperam pelo resultado e recebem a mesma resposta, evitando processamento redundante no backend.

## Escopo

- **Inclui:** Hash-based dedup; cache in-flight; janela de tempo configuravel; exclusao de streaming; campos hash_configuraveis (model, messages, temperature, max_tokens); metricas (total_unique, total_deduplicated, dedup_rate).
- **Nao inclui:** Deduplicacao de streaming (excluido por padrao).

## Descricao Funcional Detalhada

### Fluxo

1. Request A (T=0): calcula hash, verifica in-flight, nova -> processa, cacheia, entrega.
2. Request B (T=50ms): mesmo hash, em andamento -> espera, recebe mesma response.
3. Request C (T=200ms): mesmo hash, ja respondida -> retorna cacheada.

### Cenarios

1. Double-click: usuario clica "Enviar" duas vezes em 100ms.
2. Retry do cliente: retry apos timeout, request original ainda processando.
3. Multiplas instancias: dois pods enviam mesma request.
4. Polling: mesma query a cada segundo.

## Interface / Contrato

```cpp
class RequestDeduplicationPlugin : public Plugin {
    PluginResult before_request(Json::Value& body, RequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, RequestContext& ctx) override;
private:
    struct InFlightRequest {
        std::shared_future<std::string> future;
        int64_t started_at;
    };
    struct CachedResponse {
        std::string response_json;
        int64_t cached_at;
    };
    std::unordered_map<std::string, InFlightRequest> in_flight_;
    std::unordered_map<std::string, CachedResponse> recent_;
    int window_ms_ = 5000;
    int cache_ttl_ms_ = 10000;
    [[nodiscard]] std::string compute_hash(const Json::Value& body) const;
};
```

## Configuracao

```json
{
  "plugins": {
    "pipeline": [
      {
        "name": "request_dedup",
        "enabled": true,
        "config": {
          "window_ms": 5000,
          "cache_ttl_ms": 10000,
          "exclude_streaming": true,
          "hash_fields": ["model", "messages", "temperature", "max_tokens"]
        }
      }
    ]
  }
}
```

## Endpoints

N/A — plugin de pipeline. Header de resposta: X-Dedup: UNIQUE ou X-Dedup: DEDUPLICATED.

### Metricas

```json
{
  "request_dedup": {
    "total_unique": 5000,
    "total_deduplicated": 230,
    "dedup_rate": 0.044,
    "current_in_flight": 3,
    "current_cached": 12
  }
}
```

## Regras de Negocio

1. Hash SHA-256 para evitar colisao.
2. Streaming excluido por padrao (exclude_streaming: true).
3. Apenas endpoints de leitura/inferencia; nao deduplicar side effects (create key, etc).
4. Limitar tamanho e TTL do cache.
5. Segunda request espera pela primeira; mesma latencia.

## Dependencias e Integracoes

- **Internas:** Feature 10 (Plugin System).
- **Externas:** Nenhuma.

## Criterios de Aceitacao

- [ ] Hash de request baseado em hash_fields
- [ ] Requests identicas em janela compartilham response
- [ ] In-flight: segunda request espera pela primeira
- [ ] Cache: requests apos conclusao retornam cacheada
- [ ] Streaming excluido
- [ ] Metricas expostas

## Riscos e Trade-offs

1. **Streaming:** Nao deduplicar; cada conexao SSE independente.
2. **Hash collision:** Usar SHA-256 robusto.
3. **Side effects:** So deduplicar endpoints de leitura/inferencia.
4. **Memoria:** Limitar tamanho e TTL.
5. **Idempotencia:** Response identica; request_id pode confundir clientes.

## Status de Implementacao

IMPLEMENTADO — Hash-based dedup, in-flight cache, janela de tempo.

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
