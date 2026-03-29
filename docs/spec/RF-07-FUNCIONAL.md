# RF-07 — Semantic Cache

- **RF:** RF-07
- **Titulo:** Semantic Cache
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Evoluir o cache atual (match exato por hash) para um cache semantico que encontra respostas similares baseado no significado do prompt, nao no texto exato. Utiliza embeddings para calcular similaridade entre prompts e retorna respostas cacheadas quando a similaridade ultrapassa um threshold configuravel. Pode ser implementado como feature core ou como plugin do sistema de middleware.

## Escopo

- **Inclui:** SemanticCache, EmbeddingIndex, embeddings via Ollama, cosine similarity, threshold configuravel, TTL/LRU eviction, headers X-Cache (SEMANTIC-HIT/MISS), X-Semantic-Similarity, metricas em /metrics
- **Nao inclui:** ANN (approximate nearest neighbor) para volumes muito grandes, consideracao de historico de mensagens no cache

## Descricao Funcional Detalhada

### Arquitetura

O fluxo e: Request -> Calcular Embedding do Prompt -> Buscar no Cache Semantico -> Se similaridade >= threshold: Cache HIT (retornar response cacheada) | Se < threshold: Cache MISS -> Enviar ao Backend LLM -> Armazenar embedding + response no cache -> Entregar response.

Implementacao pode ser core (modificar gateway.cpp) ou plugin (SemanticCachePlugin com before_request/after_response). O plugin intercepta requests, calcula embeddings, busca por similaridade e retorna cacheado quando hit.

### Componentes

- **SemanticCache / SemanticCachePlugin:** Classe principal que armazena embeddings e respostas. Como plugin: implementa interface Plugin com hooks before_request e after_response.
- **EmbeddingIndex:** Indice de busca por similaridade (brute-force ou approximate nearest neighbor).
- **CachedEmbedding / CacheEntry:** Estrutura com embedding, response, model, created_at, last_hit_at, hit_count.

### Fluxo como Plugin

Request -> Plugin before_request -> Calcular Embedding -> Buscar Similares -> Hit (sim >= threshold): Retornar Cacheado (Block) | Miss: Continuar Pipeline -> Backend LLM -> after_response -> Armazenar Embedding + Response.

## Interface / Contrato

```cpp
struct CachedEmbedding {
    std::vector<float> embedding;
    std::string response;
    std::string model;
    int64_t created_at;
    int64_t last_hit_at;
    int hit_count;
};

class SemanticCache {
public:
    struct Config {
        bool enabled = false;
        float similarity_threshold = 0.95f;
        size_t max_entries = 10000;
        int ttl_seconds = 3600;
        std::string embedding_model = "nomic-embed-text";
    };

    void init(const Config& config);

    struct SearchResult {
        bool hit;
        float similarity;
        std::string response;
    };

    [[nodiscard]] SearchResult search(
        const std::vector<float>& query_embedding,
        const std::string& model) const;

    void store(const std::vector<float>& embedding,
               const std::string& model,
               const std::string& response);

    [[nodiscard]] Json::Value stats() const;
    void evict_expired();

private:
    [[nodiscard]] float cosine_similarity(
        const std::vector<float>& a,
        const std::vector<float>& b) const noexcept;
};

// Como Plugin (alternativa)
class SemanticCachePlugin : public Plugin {
public:
    std::string name() const override { return "semantic_cache"; }
    PluginResult before_request(Json::Value& body, RequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, RequestContext& ctx) override;
};
```

## Configuracao

### config.json (core)

```json
{
  "semantic_cache": {
    "enabled": false,
    "similarity_threshold": 0.95,
    "max_entries": 10000,
    "ttl_seconds": 3600,
    "embedding_model": "nomic-embed-text",
    "embedding_dimensions": 768
  }
}
```

### config.json (plugin)

```json
{
  "plugins": {
    "pipeline": [
      {
        "name": "semantic_cache",
        "enabled": true,
        "config": {
          "threshold": 0.95,
          "max_entries": 5000,
          "ttl_seconds": 3600,
          "embedding_model": "nomic-embed-text",
          "excluded_models": ["text-embedding-3-small"],
          "only_temperature_zero": true
        }
      }
    ]
  }
}
```

### Variaveis de Ambiente

| Variavel | Descricao | Default |
|----------|-----------|---------|
| `SEMANTIC_CACHE_ENABLED` | Habilitar cache semantico | `false` |
| `SEMANTIC_CACHE_THRESHOLD` | Threshold de similaridade (0.0-1.0) | `0.95` |
| `SEMANTIC_CACHE_MAX_ENTRIES` | Max entradas | `10000` |
| `SEMANTIC_CACHE_TTL` | TTL em segundos | `3600` |
| `SEMANTIC_CACHE_MODEL` | Modelo de embedding | `nomic-embed-text` |

## Endpoints

Nenhum endpoint novo. Headers de cache existentes sao reutilizados:

- `X-Cache: SEMANTIC-HIT` — Cache semantico hit
- `X-Cache: SEMANTIC-MISS` — Cache semantico miss
- `X-Semantic-Similarity: 0.97` — Similaridade do hit

Metricas de cache semantico expostas em `/metrics`.

## Regras de Negocio

- Similaridade calculada via cosine similarity entre embeddings.
- Hit quando similaridade >= threshold (default 0.95).
- Eviction: TTL expirado e/ou LRU quando max_entries atingido.
- Modelo de embedding obrigatorio no Ollama (ex: nomic-embed-text, all-minilm).
- Como plugin: request interna para /api/embed nao deve ativar o plugin (evitar loop infinito).
- only_temperature_zero (plugin): cachear apenas quando temperature=0 para consistencia.

## Dependencias e Integracoes

- **Internas:** OllamaClient (para calcular embeddings via /api/embed), Feature 10 (Plugin System) se implementado como plugin
- **Externas:** Nenhuma
- **Pre-requisito:** Modelo de embedding disponivel no Ollama

## Criterios de Aceitacao

- [ ] Cache semantico retorna HIT para prompts semanticamente similares (diferentes palavras, mesmo significado)
- [ ] Headers X-Cache e X-Semantic-Similarity presentes na resposta
- [ ] TTL e LRU eviction funcionam corretamente
- [ ] Metricas (hits, misses, near_misses) expostas em /metrics
- [ ] Configuracao via config.json e env vars
- [ ] Como plugin: integra no pipeline sem conflito com cache exato

## Riscos e Trade-offs

1. **Latencia do embedding:** Calcular embedding adiciona 10-50ms por request. Pode ser mais lento que o cache exato.
2. **Falsos positivos:** Threshold muito baixo pode retornar respostas erradas para perguntas diferentes.
3. **Modelo de embedding necessario:** Requer modelo carregado no Ollama. Consome memoria GPU/RAM extra.
4. **Busca O(n):** Brute-force cosine similarity em 10.000 embeddings pode ser lento. Para volumes maiores, precisa de ANN.
5. **Dimensionalidade:** Embeddings 768-1536 dims consomem memoria significativa.
6. **Contexto:** Cache semantico nao considera mensagens anteriores no historico.
7. **Plugin vs Core:** Interacao com cache exato precisa ser coordenada (ordem no pipeline).

## Status de Implementacao

IMPLEMENTADO — Cache semantico funcional, suportando embeddings, cosine similarity, threshold, TTL/LRU eviction. Implementacao como core ou plugin conforme configuracao.

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
