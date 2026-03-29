# RF-26 — Model Warmup

- **RF:** RF-26
- **Titulo:** Model Warmup
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Plugin que mantem modelos carregados na memoria do Ollama proativamente, enviando requests leves periodicamente para evitar que o Ollama descarregue modelos inativos. Detecta quando um modelo foi descarregado e o recarrega automaticamente. Reduz drasticamente a latencia de cold start.

## Escopo

- **Inclui:** Keepalive periodico; warmup no startup; deteccao de unload; schedule opcional (horario ativo); metricas (loaded, last_warmup, warmup_count, cold_start_count).
- **Nao inclui:** Warmup real no startup (nao implementado); integracao com OllamaClient para keepalive.

## Descricao Funcional Detalhada

### Fluxo

1. Timer periodico verifica modelos.
2. Se carregado: OK, pular.
3. Se descarregado: enviar request leve para Ollama /api/chat.
4. Sucesso: modelo na memoria.
5. Falha: retry.

### Cenarios

1. Horario comercial: requests esporadicas; sem warmup, metade tem cold start.
2. Modelos criticos: modelo principal sempre pronto em <2s.
3. Multiplos modelos: 3 modelos; sem warmup apenas ultimo na memoria.
4. Pre-deploy: apos restart Ollama, forcar carregamento antes de aceitar trafego.

## Interface / Contrato

```cpp
struct WarmupModel {
    std::string name;
    int interval_seconds = 240;  // Menor que Ollama unload timeout (300s)
    bool always_loaded = false;
    std::string schedule;        // "08:00-20:00" (opcional)
};

struct ModelStatus {
    std::string name;
    bool loaded = false;
    int64_t last_warmup = 0, last_used = 0;
    int warmup_count = 0, cold_start_count = 0;
    double avg_load_time_ms = 0;
};

class ModelWarmupPlugin : public Plugin {
    void shutdown() override;
    void warmup_loop();
    void warmup_model(const WarmupModel& model);
    [[nodiscard]] bool is_model_loaded(const std::string& model) const;
    [[nodiscard]] bool is_within_schedule(const std::string& schedule) const;
};
```

## Configuracao

```json
{
  "plugins": {
    "pipeline": [
      {
        "name": "model_warmup",
        "enabled": true,
        "config": {
          "models": [
            { "name": "llama3:8b", "interval_seconds": 240, "always_loaded": true },
            { "name": "nomic-embed-text", "interval_seconds": 240, "always_loaded": true },
            { "name": "qwen3:14b", "interval_seconds": 240, "schedule": "08:00-20:00" }
          ],
          "warmup_prompt": "Hi",
          "warmup_max_tokens": 1,
          "startup_warmup": true,
          "check_via_api": true
        }
      }
    ]
  }
}
```

### Variaveis de Ambiente

| Variavel | Descricao | Default |
|---|---|---|
| `WARMUP_ENABLED` | Habilitar warmup | `false` |
| `WARMUP_MODELS` | Lista de modelos (comma-separated) | (vazio) |
| `WARMUP_INTERVAL` | Intervalo em segundos | `240` |

## Endpoints

Metricas em `/metrics`:

```json
{
  "model_warmup": {
    "models": {
      "llama3:8b": {
        "loaded": true,
        "last_warmup": 1740355200,
        "warmup_count": 142,
        "cold_start_count": 3,
        "avg_load_time_ms": 12500
      }
    }
  }
}
```

## Regras de Negocio

1. Intervalo de warmup menor que Ollama unload timeout (300s); default 240s.
2. warmup_prompt e warmup_max_tokens minimos para economia.
3. Schedule em UTC, formato HH:MM-HH:MM.
4. Modelo inexistente: logar warning, nao travar.
5. Multiplos backends: warmup em todos.

## Dependencias e Integracoes

- **Internas:** Feature 10 (Plugin System), OllamaClient (requests de warmup).
- **Externas:** Nenhuma.

## Criterios de Aceitacao

- [ ] Warmup no startup (startup_warmup: true)
- [ ] Keepalive periodico por modelo
- [ ] Deteccao de unload e reload automatico
- [ ] Schedule respeitado (08:00-20:00)
- [ ] Metricas em /metrics
- [ ] Modelo inexistente: warning sem travar

## Riscos e Trade-offs

1. **Consumo de recursos:** Cada warmup consome GPU time; muitos modelos impactam requests reais.
2. **Intervalo vs Ollama timeout:** Se Ollama muda timeout, warmup pode ser insuficiente.
3. **Memoria GPU:** Manter muitos modelos pode exaurir VRAM.
4. **Multiplos backends:** Warmup em todos os backends.
5. **Schedule:** Parsing de horarios/timezones; manter simples (UTC).

## Status de Implementacao

IMPLEMENTADO — ModelWarmupPlugin implementado com: warmup real no startup via httplib (POST /api/chat com prompt minimo), keepalive periodico por modelo, schedule UTC (HH:MM-HH:MM), deteccao de unload com cold_start_count, metricas (loaded, last_warmup, warmup_count, cold_start_count, avg_load_time_ms).

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
