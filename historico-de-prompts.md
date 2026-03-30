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

### Prompt 6

tag commit and push

**Resultado:** Commit `a3021ca` (apenas fontes/plugins/scripts/histórico; `.o` rastreados restaurados para não versionar build local). Tag `v2.0.4-ci-unit-tests`. `git push main master` e `git push main v2.0.4-ci-unit-tests`.

### Prompt 7

Implementar cache de dependências (docs GitHub Actions) e actions/attest-build-provenance no workflow.

**Resultado:** Em `.github/workflows/deploy.yml`: job `test` com `actions/cache@v4` em `.apt-cache` e `Dir::Cache::Archives` via `apt.conf`; job `build-push-deploy` com cache do binário Kustomize, `docker/setup-buildx-action` + `cache-from`/`cache-to` type=gha no build-push; permissões `id-token` e `attestations` só no deploy; passo `actions/attest-build-provenance@v4` com `subject-digest` do build e `push-to-registry: true`.

### Prompt 8

Erro de link LTO: undefined reference a `FinOpsPlugin`, `DataDiscoveryPlugin`, `DLPPlugin`, `GuardrailsPlugin` (vtable/typeinfo).

**Resultado:** Inclusão de `guardrails_plugin.cpp`, `data_discovery_plugin.cpp`, `dlp_plugin.cpp` e `finops_plugin.cpp` em `ENTERPRISE_PLUGIN_SOURCES` no `Makefile` (já usados em `gateway.cpp`, mas não linkados no alvo `hermes`).

### Prompt 9

Criar `.gitignore` coerente para o projeto; não versionar `.cursor`.

**Resultado:** Adicionado `.gitignore` na raiz cobrindo `.cursor`, binários C++ (`hermes`, `*.o`, `*.so`, etc.), diretórios CMake (`build*/`, artefatos), `node_modules`/build front, Python venv/cache, `.env`, logs e IDEs comuns.

### Prompt 10

Não quero nada de `.claude/`.

**Resultado:** `.gitignore` atualizado para ignorar `.claude/` e `.claude` por completo (removida exceção que só ignorava `settings.local.json`).

### Prompt 11

Não quero nada de `.gitmodules`.

**Resultado:** `.gitmodules` adicionado ao `.gitignore`; submodule `.cursor` desinicializado e removido do repositório (`git submodule deinit`, `git rm .cursor`); `git rm .gitmodules` para deixar de versionar o ficheiro.

### Prompt 12

Incluir no README os ribbons (badges) do projeto.

**Resultado:** Bloco de badges após o título `# HERMES`: CI/CD (workflow `deploy.yml`), licença Apache 2.0, C++23, imagem GHCR/Docker, estrelas e forks no GitHub (`pipenodes/llm-gateway`).

### Prompt 13

Completar README com ribbons de Kubernetes, attestation GitHub e afins.

**Resultado:** Segunda linha de badges: Artifact Attestations (link `.../attestations`), SLSA, Sigstore, Prometheus, OpenAPI, Ollama; primeira linha ganhou Kubernetes e Kustomize (links para `k8s/`). Parágrafo `<sub>` em PT sobre deploy Kustomize, attestations no GHCR e doc oficial PT + `gh attestation verify`.

### Prompt 14

tag commit and push

**Resultado:** Commit `d1c2ecf` (gitignore, remoção submodule `.cursor`/`.gitmodules`, workflow, README badges, Makefile enterprise, `SOLUTION_SUMMARY` em `docs/`, etc.); tag `v2.0.5-ci-readme-gitignore`; `git push main master` e push da tag.

### Prompt 15

CI: falha ao gravar cache APT (`lock`/`partial` permission denied); warning `No url found for submodule ... googletest-src`.

**Resultado:** Após `apt-get install`, `sudo chown -R "$USER:$USER" .apt-cache` no `deploy.yml`. Removido gitlink `build-e2e/_deps/googletest-src` do índice (`git rm --cached`); entrada em `.gitignore` para `build-e2e/_deps/googletest-src/` (FetchContent, sem .gitmodules).

### Prompt 16

Deploy só em tags + TAG.md / CHANGELOG.md + regra Cursor (implementar plano: CI separado, release por tag `v*`, TAG/CHANGELOG, regra `.cursor/rules/release-and-tags.mdc`, README/CONTRIBUTING).

**Resultado:** Criado `.github/workflows/ci.yml` (testes em PR e push `main`/`master`). `deploy.yml` como workflow **Release**: `on` em tags `v*` e `workflow_dispatch`; verificação `merge-base` contra `origin/master` ou `origin/main`; imagem com SHA, `latest` e nome da tag só em push de tag; Kustomize mantém imagem por SHA. Adicionados `TAG.md`, `CHANGELOG.md`, `.cursor/rules/release-and-tags.mdc` (`alwaysApply`). README com badges CI/Release, parágrafo de deploy e secção **Release**; CONTRIBUTING com secção **Releases**. Entrada neste histórico.

