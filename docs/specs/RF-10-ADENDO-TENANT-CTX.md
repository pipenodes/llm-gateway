# RF-10 — Adendo: Contexto de Tenant no RequestContext

- **RF:** RF-10 (adendo)
- **Titulo:** Contexto Multi-Tenant via ctx.metadata
- **Autor:** HERMES Team
- **Data:** 2026-03-23
- **Versao:** 1.0
- **Status:** RASCUNHO

**Referencia principal:** RF-10-FUNCIONAL.md (Plugin/Middleware System)

## Motivacao

Os plugins de segurança e governança (RF-32 GuardRails, RF-33 DLP, RF-34 Data Discovery, RF-35 FinOps) requerem acesso ao triplete de identidade `tenant_id / app_id / client_id` em toda execução de pipeline. O `RequestContext` atual (RF-10) não possui esses campos explícitos — o único identificador de origem é `key_alias`.

Este adendo define como o **gateway core** deve popular esses valores **antes de iniciar o pipeline de plugins**, usando o campo `metadata` já existente no `RequestContext`. A estratégia é retrocompatível: nenhuma interface de plugin existente é quebrada.

## Contrato

### Headers de entrada mapeados pelo core

| Header HTTP | Campo em ctx.metadata | Fallback |
|-------------|----------------------|----------|
| `x-tenant` | `ctx.metadata["tenant_id"]` | `"default"` |
| `x-project` | `ctx.metadata["app_id"]` | `"default"` |
| *(da API key autenticada)* | `ctx.metadata["client_id"]` | `ctx.key_alias` |

### Ponto de populacao no gateway core

O core deve popular `ctx.metadata` imediatamente após construir o `RequestContext` e **antes** de chamar `plugin_manager.run_before_request()`:

```cpp
// gateway.cpp — handler de /v1/chat/completions (e demais endpoints)
RequestContext ctx;
ctx.request_id = generate_uuid();
ctx.key_alias  = resolved_key_alias;   // já preenchido pela autenticação
ctx.client_ip  = req.remote_addr;
ctx.model      = body["model"].asString();
ctx.method     = req.method;
ctx.path       = req.path;
ctx.is_stream  = body.get("stream", false).asBool();

// --- Adendo: contexto multi-tenant ---
auto tenant = req.get_header_value("x-tenant");
auto app    = req.get_header_value("x-project");
ctx.metadata["tenant_id"] = tenant.empty() ? "default" : tenant;
ctx.metadata["app_id"]    = app.empty()    ? "default" : app;
ctx.metadata["client_id"] = ctx.key_alias;
// ------------------------------------

plugin_manager.run_before_request(body, ctx);
```

### Helper inline (disponibilizado em plugin.h ou tenant_ctx.h)

Para evitar duplicação nos plugins, expor utilitários inline no header compartilhado:

```cpp
// src/tenant_ctx.h  (novo arquivo, incluído em plugin.h)
#pragma once
#include "plugin.h"

inline std::string ctx_tenant(const RequestContext& ctx) {
    auto it = ctx.metadata.find("tenant_id");
    return (it != ctx.metadata.end() && !it->second.empty())
        ? it->second : "default";
}

inline std::string ctx_app(const RequestContext& ctx) {
    auto it = ctx.metadata.find("app_id");
    return (it != ctx.metadata.end() && !it->second.empty())
        ? it->second : "default";
}

// client_id == ctx.key_alias (acesso direto, sem helper necessário)
```

## Invariantes

1. `tenant_id` e `app_id` são sempre strings não-vazias após população — nunca `""`.
2. O fallback `"default"` garante backward compatibility com deployments single-tenant que não enviam os headers.
3. Plugins existentes (RF-13, RF-14, RF-15, RF-20, etc.) **não são afetados** — ignoram `metadata` que não conhecem.
4. `client_id` em `ctx.metadata` é redundante com `ctx.key_alias` — existe para uniformidade de interface nos novos plugins; sempre devem ser idênticos.
5. Plugins **nunca** devem popular `tenant_id` ou `app_id` no `metadata` — responsabilidade exclusiva do gateway core.

## Validacao em init() dos novos plugins

Plugins que dependem do contexto de tenant devem logar um warning em `init()` quando os headers não estiverem presentes na configuração do gateway (modo de detecção: verificar env/config, não o valor em runtime):

```cpp
bool GuardrailsPlugin::init(const Json::Value& config) {
    // ... carregar policies ...
    LOG_INFO("guardrails: tenant context via ctx.metadata[tenant_id/app_id]. "
             "Set x-tenant and x-project headers for multi-tenant isolation. "
             "Fallback: 'default' for single-tenant deployments.");
    return true;
}
```

## Seguranca

- Os headers `x-tenant` e `x-project` são **não autenticados por padrão** — qualquer cliente pode enviar qualquer valor.
- Em ambientes multi-tenant reais, o gateway deve **validar e resolver** o tenant a partir da API key autenticada (não do header), usando uma tabela `key_alias → tenant_id` no ApiKeyManager. Esta validação é **trabalho futuro** — nesta versão, o header é aceito como declarado.
- Enquanto a validação não existir, deployments multi-tenant devem garantir que apenas clientes confiáveis (internos) possam setar `x-tenant` — via IP whitelist ou autenticação mTLS no ingresso.

## Impacto nos RFs existentes

| RF | Impacto |
|----|---------|
| RF-01 (Multi-Provider) | Nenhum — ProviderRouter não usa tenant_id |
| RF-02 (Usage Tracking) | Nenhum — opera por key_alias |
| RF-10 (Plugin System) | Este adendo — sem mudança de interface |
| RF-13 (PII Redaction) | Nenhum — ignora metadata de tenant |
| RF-14 (Content Moderation) | Nenhum |
| RF-15 (Prompt Injection) | Nenhum |
| RF-20 (Cost Controller) | Nenhum — opera por key_alias |
| RF-21 (Rate Limiter) | Nenhum |
| RF-32 (GuardRails) | **Depende** — usa ctx_tenant() e ctx_app() |
| RF-33 (DLP) | **Depende** — usa ctx_tenant() e ctx_app() |
| RF-34 (Data Discovery) | **Depende** — usa ctx_tenant() e ctx_app() |
| RF-35 (FinOps) | **Depende** — usa ctx_tenant() e ctx_app() |

## Criterios de Aceitacao

- [ ] Gateway popula `ctx.metadata["tenant_id"]`, `ctx.metadata["app_id"]` e `ctx.metadata["client_id"]` antes de `run_before_request()`
- [ ] Fallback para `"default"` quando headers ausentes
- [ ] `ctx_tenant()` e `ctx_app()` disponíveis em `src/tenant_ctx.h`
- [ ] Plugins existentes (RF-13/14/15/20/21) não quebram com a adição dos campos em metadata
- [ ] Plugins novos (RF-32/33/34/35) lêem tenant via helper, nunca diretamente do header HTTP
- [ ] Warning logado em `init()` dos novos plugins documentando o mecanismo
