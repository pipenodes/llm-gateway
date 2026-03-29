# RF-07-AI — Semantic Cache

- **RF:** RF-07
- **Titulo:** Semantic Cache
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

**Referencia funcional:** RF-07-FUNCIONAL.md
**Referencia UX:** N/A (backend only)

## 1. Visao geral do comportamento esperado

O componente de cache semantico utiliza modelos de embedding para converter prompts em vetores de alta dimensao e busca respostas cacheadas por similaridade coseno. Quando a similaridade entre o prompt da request e um prompt cacheado ultrapassa o threshold configurado, retorna a resposta cacheada sem acionar o backend LLM.

**Limites conhecidos:**
- O cache nao considera o historico de mensagens (context window). Duas conversas diferentes com a mesma ultima mensagem podem ter respostas diferentes, mas o cache retornaria a mesma.
- Busca brute-force O(n) em todas as entradas; para volumes muito grandes, ANN (approximate nearest neighbor) seria necessario.
- Threshold muito baixo gera falsos positivos (ex.: "capital do Brasil" vs "capital da Argentina" podem ter similaridade alta).
- Latencia do embedding adiciona 10-50ms por request.

### Comportamentos minimos

- Calcular embedding do prompt via modelo configurado (ex.: nomic-embed-text).
- Buscar no indice por similaridade coseno entre query e entradas cacheadas do mesmo modelo.
- Retornar HIT quando similaridade >= threshold; caso contrario MISS e encaminhar ao backend.
- Armazenar embedding + response no cache apos resposta do backend (apenas em MISS).
- Evict por TTL e LRU quando max_entries excedido.
- Expor headers X-Cache (SEMANTIC-HIT/MISS) e X-Semantic-Similarity no hit.

## 2. Intents / comandos suportados

| Intent | Descricao |
|--------|-----------|
| **Cache lookup** | Buscar resposta similar no cache antes de enviar ao LLM |
| **Cache store** | Armazenar nova resposta com embedding apos MISS |
| **Eviction** | Remover entradas expiradas ou LRU quando necessario |

Nao ha comandos explicitos do usuario; o comportamento e transparente nas requests de chat completions.

## 3. Contratos API (minimo)

Nenhum endpoint novo. O cache semantico intercepta requests existentes em `/v1/chat/completions`.

**Headers de resposta (cache hit):**
- `X-Cache: SEMANTIC-HIT` ou `X-Cache: SEMANTIC-MISS`
- `X-Semantic-Similarity: <float>` (apenas em HIT)

**Metricas expostas em `/metrics`:**
- `semantic_cache_hits_total`
- `semantic_cache_misses_total`
- `semantic_cache_near_misses_total` (similaridade > 0.8 mas < threshold)

**Embedding interno:** Utiliza endpoint de embeddings do gateway (ex.: `/api/embed` ou equivalente) para calcular vetores. A request de embedding nao deve ativar o plugin de cache novamente (evitar loop).

## 4. Fluxos detalhados

### Fluxo 1: Request com cache MISS

1) Request chega em `/v1/chat/completions` com messages.
2) Plugin/feature extrai texto do prompt (ultima mensagem user ou concatenacao).
3) Calcula embedding do texto via modelo configurado.
4) Busca no indice por similaridade coseno com entradas do mesmo modelo.
5) Nenhuma entrada com similaridade >= threshold.
6) Encaminha request ao backend LLM.
7) Recebe response do backend.
8) Armazena embedding + response + model no cache.
9) Retorna response ao cliente com header `X-Cache: SEMANTIC-MISS`.

### Fluxo 2: Request com cache HIT

1) Request chega em `/v1/chat/completions` com messages.
2) Plugin/feature extrai texto do prompt.
3) Calcula embedding do texto.
4) Busca no indice; encontra entrada com similaridade 0.97 (>= threshold 0.95).
5) Retorna response cacheada imediatamente (sem acionar backend).
6) Atualiza last_hit_at e hit_count da entrada.
7) Retorna ao cliente com `X-Cache: SEMANTIC-HIT` e `X-Semantic-Similarity: 0.97`.

### Fluxo 3: Eviction

1) Periodicamente ou apos store, verifica se max_entries excedido ou entradas expiradas.
2) Remove entradas com created_at + ttl_seconds < now.
3) Se ainda acima de max_entries, remove entradas por LRU (menor last_hit_at).

## 5. Validacoes e guardrails

- **Threshold de similaridade:** Valor entre 0.0 e 1.0. Default 0.95. Valores muito baixos aumentam risco de falsos positivos (resposta errada para pergunta diferente).
- **Modelo de embedding:** Deve estar disponivel no Ollama ou provider configurado. Falha ao obter embedding resulta em MISS e log de erro.
- **Exclusao de requests de embedding:** Requests internas para calcular embedding nao devem passar pelo semantic cache (evitar loop infinito).
- **only_temperature_zero (plugin):** Opcionalmente cachear apenas quando temperature=0 para garantir determinismo da resposta.
- **excluded_models:** Lista de modelos que nao devem usar cache semantico.

## 6. Seguranca e autorizacao

- O cache semantico nao altera RBAC. Autorizacao e feita antes do plugin.
- Respostas cacheadas sao retornadas sem revalidacao de permissao (assume que a mesma key/model teria acesso).
- Nao armazenar conteudo sensivel em logs de metricas; apenas contadores e similaridade.

## 7. Telemetria e observabilidade

### Eventos recomendados

- `semantic_cache_lookup` (hit/miss, similarity, model)
- `semantic_cache_store` (model, entry_count)
- `semantic_cache_eviction` (reason: ttl|lru, entries_removed)
- `semantic_cache_embedding_error` (model, error_message)

### Metadata minima

- traceId (X-Request-Id)
- model (modelo do chat)
- embedding_model (modelo de embedding usado)
- similarity (em hit)
- key_alias (para correlacao com uso)

## 8. Criterios de aceitacao (PoC/MVP)

- [ ] Prompt "Qual a capital do Brasil?" gera MISS e cacheia resposta.
- [ ] Prompt "Me diga a capital brasileira" com mesmo modelo gera HIT com similaridade >= 0.90.
- [ ] Header X-Cache e X-Semantic-Similarity presentes em respostas.
- [ ] Metricas hits/misses/near_misses expostas em /metrics.
- [ ] TTL e max_entries respeitados (eviction funciona).
- [ ] Request de embedding nao ativa o cache (sem loop).

## Checklist de qualidade da especificacao AI

- [x] Intents mapeadas para fluxos reais
- [x] Guardrails e RBAC explicitos
- [x] Contratos API minimos definidos
- [x] Eventos e metadata de observabilidade definidos
- [x] Criterios de aceitacao objetivos e testaveis
