# RF-08 — A/B Testing de Modelos

- **RF:** RF-08
- **Titulo:** A/B Testing de Modelos
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Permitir split de trafego entre dois ou mais modelos para comparar qualidade, latencia e custo de forma controlada. O gateway roteia uma porcentagem configuravel de requests para cada modelo e coleta metricas comparativas, tudo transparente para o cliente. Suporta 2 a 5 modelos por experimento e modelos de diferentes providers (ollama, openrouter, custom).

## Escopo

- **Inclui:** ABTestConfig, ABRouter, ABMetrics, ABExperiment, 2-5 variantes por experimento, weighted routing, metricas por variante (request_count, tokens, latencia p50/p95/p99, error_count), endpoints admin
- **Nao inclui:** Avaliacao de qualidade automatica (LLM-as-judge), sticky routing por session, benchmark ad-hoc (ver RF-11)

## Descricao Funcional Detalhada

### Arquitetura

Request (model: auto ou trigger_model) -> A/B Router -> Escolhe variante por peso (ex: 70% Model A, 30% Model B) -> Roteia para modelo escolhido -> Registra metricas por variante -> Comparacao agregada disponivel em /admin/ab.

### Componentes

- **ABExperiment:** Configuracao de um experimento (name, trigger_model, variants com model/provider/weight).
- **ABRouter:** Decide qual modelo usar baseado em pesos configurados (weighted random).
- **ABMetrics / ABVariantMetrics:** Coleta metricas separadas por variante (request_count, total_tokens, avg_latency_ms, p50/p95/p99, error_count).

### Regras de Configuracao

- Minimo 2 variantes, maximo 5 por experimento.
- Pesos devem somar 100 (validacao ao carregar).
- Cada variante pode especificar provider; se omitido, usa model_routing.

## Interface / Contrato

```cpp
struct ABExperiment {
    std::string name;
    std::string trigger_model;
    struct Variant {
        std::string model;
        std::string provider;
        int weight;
    };
    std::vector<Variant> variants;
    bool enabled = true;
    int64_t created_at;
};

struct ABVariantMetrics {
    std::string model;
    int64_t request_count = 0;
    int64_t total_tokens = 0;
    double avg_latency_ms = 0;
    double p50_latency_ms = 0;
    double p95_latency_ms = 0;
    double p99_latency_ms = 0;
    int64_t error_count = 0;
};

class ABTesting {
public:
    void init(const std::string& config_path);
    [[nodiscard]] std::optional<std::string> resolve(
        const std::string& requested_model,
        const std::string& request_id) const;
    void record(const std::string& experiment_name,
                const std::string& variant_model,
                int tokens, double latency_ms, bool error);
    void add_experiment(const ABExperiment& exp);
    void remove_experiment(const std::string& name);
    [[nodiscard]] Json::Value get_results(const std::string& name) const;
    [[nodiscard]] Json::Value list_experiments() const;
};
```

## Configuracao

### config.json

```json
{
  "ab_testing": {
    "enabled": true,
    "experiments": [
      {
        "name": "gemma-comparison",
        "trigger_model": "default",
        "variants": [
          { "model": "gemma3:1b", "provider": "ollama", "weight": 34 },
          { "model": "gemma3:4b", "provider": "ollama", "weight": 33 },
          { "model": "gemma3:12b", "provider": "ollama", "weight": 33 }
        ]
      },
      {
        "name": "multi-provider-test",
        "trigger_model": "gpt-4",
        "variants": [
          { "model": "gemma3:4b", "provider": "ollama", "weight": 50 },
          { "model": "openai/gpt-4o-mini", "provider": "openrouter", "weight": 50 }
        ]
      }
    ]
  }
}
```

### Variaveis de Ambiente

| Variavel | Descricao | Default |
|----------|-----------|---------|
| `AB_TESTING_ENABLED` | Habilitar A/B testing | `false` |

## Endpoints

| Metodo | Path | Descricao |
|--------|------|-----------|
| `GET` | `/admin/ab` | Listar experimentos e resultados |
| `GET` | `/admin/ab/{name}` | Resultados de um experimento |
| `POST` | `/admin/ab` | Criar experimento |
| `DELETE` | `/admin/ab/{name}` | Remover experimento |

### Response de /admin/ab/{name}

```json
{
  "name": "llama-vs-gemma",
  "trigger_model": "default",
  "variants": [
    {
      "model": "llama3:8b",
      "weight": 70,
      "metrics": {
        "request_count": 700,
        "total_tokens": 245000,
        "avg_latency_ms": 1250,
        "p95_latency_ms": 3200,
        "error_rate": 0.02
      }
    },
    {
      "model": "gemma3:4b",
      "weight": 30,
      "metrics": {
        "request_count": 300,
        "total_tokens": 98000,
        "avg_latency_ms": 890,
        "p95_latency_ms": 2100,
        "error_rate": 0.01
      }
    }
  ]
}
```

## Regras de Negocio

- Resolucao por weighted random: peso N significa ~N% do trafego.
- Header X-AB-Variant retornado na resposta indica qual variante foi usada.
- Requests cacheadas excluidas das metricas A/B.
- Variante escolhida registrada no header da primeira resposta em streaming.
- Percentis (p50/p95/p99) requerem armazenar valores historicos (reservatorio ou digest).

## Dependencias e Integracoes

- **Internas:** Config (model aliases, model_routing, providers), ProviderRouter, MetricsCollector
- **Externas:** Nenhuma

## Criterios de Aceitacao

- [ ] ABRouter roteia requests conforme pesos configurados
- [ ] Metricas por variante coletadas e expostas em /admin/ab/{name}
- [ ] Minimo 2, maximo 5 variantes; pesos somam 100
- [ ] Suporte a modelos de diferentes providers
- [ ] Header X-AB-Variant presente na resposta
- [ ] Endpoints admin protegidos por Authorization

## Riscos e Trade-offs

1. **Determinismo:** Mesmo cliente pode receber modelos diferentes em requests consecutivas. Opcao futura: sticky routing por key alias ou session.
2. **Metricas de qualidade:** Latencia e tokens sao faceis de medir. Qualidade requer avaliacao humana ou LLM-as-judge (ver RF-11).
3. **Interacao com cache:** Requests cacheadas nao testam o modelo; excluir das metricas.
4. **Streaming:** Variante precisa ser registrada no header da primeira resposta.
5. **Percentil de latencia:** Calcular p50/p95/p99 requer armazenar historico; usar reservatorio ou digest.

## Status de Implementacao

IMPLEMENTADO — ABTestingPlugin implementado com: weighted random routing, 2-5 variantes por experimento, metricas por variante (request_count, total_tokens, p50/p95/p99 latencia, error_rate), IAuditSink integration, header X-AB-Variant, endpoints GET/POST/DELETE /admin/ab, GET /admin/ab/{name}. Header X-AB-Variant agora enviado corretamente em respostas streaming via applyPluginHeaders antes de set_chunked_content_provider; aplicado aos handlers de chat stream e completion stream.

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
