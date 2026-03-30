# Histórico de Prompts

## 2026-03-23

### Prompt 1

Faça uma revisão técnica e de produto completa da minha solução comparando com concorrentes de mercado e salvando em docs/POSICIONAMENTO.md

**Resultado:** Criado `docs/POSICIONAMENTO.md` com revisão completa cobrindo: panorama do mercado (LiteLLM, Portkey, Bifrost, Kong AI, Cloudflare, OpenRouter, Olla), matriz de comparação de features, análise crítica de pontos fortes e lacunas, posicionamento estratégico recomendado, personas ideais, roadmap priorizado e comparações narrativas com cada concorrente.

## 2026-03-30

### Prompt 1

Continuar correção do CI llm-gateway: includes de plugins após reorganização core/enterprise.

**Resultado:** `gateway.cpp` atualizado com `#include` para `plugins/core/*` e `plugins/enterprise/*`; `gateway.h` já apontava para os caminhos corretos.

### Prompt 2

Run make all — erro no pipeline: `logging_plugin.h` fatal error `../plugin.h: No such file or directory`.

**Resultado:** Trocado `#include "../plugin.h"` por `#include "plugin.h"` em todos os headers sob `src/plugins/core/` e `src/plugins/enterprise/`, alinhado ao `-Isrc` do Makefile e à localização real de `src/plugin.h` (o `../` apontava para `src/plugins/plugin.h`, inexistente).

### Prompt 3

git tag commit and push

**Resultado:** Commit `68d0968` com a correção de includes; tag anotada `v2.0.3-plugin-includes`; `git push main master` e `git push main v2.0.3-plugin-includes` (remote nomeado `main`, não `origin`).

### Prompt 4

Erro CI: `semantic_cache_plugin.h` fatal error `../ollama_client.h: No such file or directory`.

**Resultado:** Em `src/plugins/**`, substituídos includes `../` e `../../tenant_ctx.h` que apontavam para headers em `src/` por `#include "…"` (via `-Isrc`): `ollama_client.h`, `core_services.h`, `request_queue.h`, `rate_limiter.h`, `cache.h`, `api_keys.h`, `logger.h`, `crypto.h`, `httplib.h`, `tenant_ctx.h`.

### Prompt 5

Revisar se não faltou mais nada para os testes darem certo antes de commitar; teste local.

**Resultado:** Renomeadas structs duplicadas em headers enterprise (`PromptInjectionPattern`, `GuardrailsInjectionPattern`, `CostControllerModelPricing`, `FinOpsModelPricing`) para `make all` após includes corrigidos. `flush_loop` em data_discovery, finops, cost_controller e usage_tracking passa a dormir em passos de 1s com checagem de `running_`, para `shutdown()`/`join()` não bloquearem até `flush_interval_seconds_`. `dlp_plugin.cpp`: removido `asStringRef()` (JsonCpp antigo do Ubuntu). `scripts/unit-tests/CMakeLists.txt`: linka `.cpp` dos plugins + OpenSSL + `CPPHTTPLIB_OPENSSL_SUPPORT`, baixa `httplib.h` se ausente, `gtest_discover_tests` com `DISCOVERY_MODE PRE_TEST`, CMake ≥3.18. `test_security_plugins.cpp`: regexes de discovery mais seguras para `std::regex`/libstdc++. Validado no WSL: 25 testes passando; `make all` local falhou só por falta de `libyaml-cpp-dev` no WSL (CI já instala).

