# RF-11 — Benchmark Ad-hoc

- **RF:** RF-11
- **Titulo:** Benchmark Ad-hoc
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Endpoint para executar benchmarks ad-hoc de modelos LLM, sem necessidade de configurar A/B Testing. Permite comparar 2 a 5 modelos (de providers diferentes) em ate 10 prompts, com parametrizacao identica (temperatura, max_tokens, etc.). O resultado inclui metricas de performance (latencia, tokens) e qualidade (avaliacao via LLM-as-judge com rubrica completa).

## Escopo

- **Inclui:** POST /v1/benchmark, GET /v1/benchmark/{job_id}, 2-5 modelos, 1-10 prompts, params globais, evaluator (LLM-as-judge), rubrica JSON (score, relevance, coherence, conciseness, reason), modo sync/async
- **Nao inclui:** A/B Testing em producao (ver RF-08), benchmark continuo ou agendado

## Descricao Funcional Detalhada

### Arquitetura

POST /v1/benchmark -> Parse request -> Para cada modelo (2-5) -> Para cada prompt (1-10) -> chat_completion -> Registrar performance -> Chamar modelo avaliador (LLM-as-judge) -> Rubrica JSON -> Agregar resultados -> Response JSON.

### Fluxo Modo Async

POST /v1/benchmark?async=true -> Retorna job_id imediatamente -> Benchmark roda em background -> GET /v1/benchmark/{job_id} retorna resultado quando completed ou progresso quando running.

### Componentes

- **Benchmark:** Orquestra execucao, coleta metricas, chama avaliador.
- **Evaluator:** Modelo LLM que avalia cada resposta (LLM-as-judge) retornando JSON com score, relevance, coherence, conciseness, reason.
- **Rubrica:** Template interno do prompt do avaliador; fallback se JSON invalido (score 0, reason "parse error").

## Interface / Contrato

Request body: models (array 2-5 com model/provider), prompts (array 1-10 strings), params (temperature, max_tokens), evaluator (model, provider).

Response: job_id, status (completed/running), results por modelo com prompts (response, performance, evaluation), aggregate (avg_latency_ms, avg_score, total_tokens), summary (best_model, fastest_model, highest_quality_model).

## Configuracao

### config.json

```json
{
  "benchmark": {
    "enabled": true,
    "max_models": 5,
    "max_prompts": 10,
    "default_evaluator": {
      "model": "gemma3:8b",
      "provider": "ollama"
    },
    "default_params": {
      "temperature": 0.7,
      "max_tokens": 256
    },
    "max_concurrent_jobs": 2,
    "sync_timeout_seconds": 600
  }
}
```

O request do endpoint pode sobrescrever esses defaults.

## Endpoints

| Metodo | Path | Descricao |
|--------|------|-----------|
| `POST` | `/v1/benchmark` | Executa benchmark. Sync por padrao; ?async=true retorna job_id |
| `GET` | `/v1/benchmark/{job_id}` | Consulta resultado (quando async) |

### Request Body

```json
{
  "models": [
    { "model": "gemma3:1b", "provider": "ollama" },
    { "model": "gemma3:4b", "provider": "ollama" },
    { "model": "openai/gpt-4o-mini", "provider": "openrouter" }
  ],
  "prompts": [
    "Explique recursao em uma frase.",
    "Qual a capital do Brasil?"
  ],
  "params": { "temperature": 0.7, "max_tokens": 256 },
  "evaluator": { "model": "gemma3:8b", "provider": "ollama" }
}
```

### Response (exemplo)

```json
{
  "job_id": "bench_xxx",
  "status": "completed",
  "results": [
    {
      "model": "gemma3:1b",
      "provider": "ollama",
      "prompts": [
        {
          "prompt_index": 0,
          "response": "...",
          "performance": { "latency_ms": 450, "prompt_tokens": 12, "completion_tokens": 25 },
          "evaluation": { "score": 8, "relevance": 9, "coherence": 8, "conciseness": 7, "reason": "Resposta correta e clara." }
        }
      ],
      "aggregate": { "avg_latency_ms": 420, "avg_score": 7.8, "total_tokens": 450 }
    }
  ],
  "summary": {
    "best_model": "gemma3:4b",
    "fastest_model": "gemma3:1b",
    "highest_quality_model": "gemma3:12b"
  }
}
```

## Regras de Negocio

- Rubrica do avaliador: retornar APENAS JSON valido com score (1-10), relevance, coherence, conciseness (1-10 cada), reason (string).
- Se avaliador retornar JSON invalido: fallback score 0, reason "parse error".
- max_concurrent_jobs: limitar jobs async simultaneos (ex: 1-2) para nao sobrecarregar backends.
- sync_timeout_seconds: timeout para modo sync (default 600).

## Dependencias e Integracoes

- **Internas:** ProviderRouter, Config, RequestQueue (opcional)
- **Externas:** Nenhuma

## Criterios de Aceitacao

- [ ] POST /v1/benchmark executa benchmark com 2-5 modelos e 1-10 prompts
- [ ] Modo sync retorna resultado completo; modo async retorna job_id
- [ ] GET /v1/benchmark/{job_id} retorna resultado quando completed
- [ ] LLM-as-judge avalia cada resposta com rubrica JSON
- [ ] Fallback para JSON invalido do avaliador
- [ ] summary com best_model, fastest_model, highest_quality_model

## Riscos e Trade-offs

1. **Timeout:** Benchmark longo pode estourar timeout HTTP; modo async reduz risco.
2. **Parse do avaliador:** LLM pode retornar JSON invalido; fallback e retry implementados.
3. **Concorrencia:** Limitar jobs async para evitar sobrecarga.
4. **Custo:** 5 modelos x 10 prompts = 50 chamadas + 50 avaliacoes = 100 chamadas LLM.

## Status de Implementacao

IMPLEMENTADO — Endpoint POST /v1/benchmark, GET /v1/benchmark/{job_id}, comparacao de modelos, LLM-as-judge, categorias, modo sync/async. Script benchmark_sync.sh para validacao.

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
