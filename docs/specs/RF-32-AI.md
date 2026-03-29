# RF-32-AI — GuardRails (LLM Firewall)

- **RF:** RF-32
- **Titulo:** GuardRails — LLM Firewall
- **Autor:** HERMES Team
- **Data:** 2026-03-23
- **Versao:** 1.0
- **Status:** RASCUNHO

**Referencia funcional:** RF-32-FUNCIONAL.md
**Referencia UX:** N/A (backend only)

## 1. Visao geral do comportamento esperado

O plugin GuardRails opera em três camadas de análise com custo e profundidade progressivos. L1 é deterministico e obrigatório. L2 usa modelos ML (ONNX) para classificação de toxicidade, jailbreak e NER — executado sincronamente mas com `sample_rate` configurável por tenant para controle de custo. L3 usa um LLM como juiz para análise semântica adversarial — executado assincronamente, sem impacto na latência do cliente.

**Limites conhecidos:**
- L2 ONNX adiciona 50-200ms de latência; manter `sample_rate < 1.0` em produção de alta carga.
- L3 LLM Judge consome créditos de backend; usar `sample_rate ≤ 0.1` por padrão.
- L2/L3 não operam sobre chunks individuais de streaming SSE — apenas sobre body completo.
- Modelos ONNX precisam ser compatíveis com o schema de input esperado pelo classificador.
- Thresholds ML precisam de calibração por tenant para minimizar falsos positivos.
- Padrões em L1 são em inglês por padrão; multilinguagem exige patterns adicionais por tenant.

### Comportamentos minimos

- L1 sempre executa para todo request independente de configuração de L2/L3.
- L2 executa quando `l2.enabled: true` e request passa no sorteio de `l2.sample_rate`.
- L3 agenda análise assíncrona quando `l3.enabled: true` após response enviada ao cliente.
- Scoring combinado em L2: `score = max(toxicity_score, jailbreak_score * 0.9)`.
- Threshold de bloqueio configurável por tenant; PolicyBundle `"default"` como fallback global.
- Todo evento de resultado inclui `{tenant_id, app_id, client_id, layer, score, verdict}`.
- Fallback para `tenant_id="default"` quando header `x-tenant` ausente.

## 2. Intents / comportamentos mapeados

| Intent | Camada | Descricao |
|--------|--------|-----------|
| **Payload guard** | L1 | Bloquear requests acima do limite de tamanho por tenant |
| **Model access control** | L1 | Permitir/negar modelos por tenant+app (allow/deny list) |
| **Rate enforcement** | L1 | Limitar RPM por tenant:client_id via token bucket |
| **Injection detection** | L1 | Score ponderado regex para prompt injection (patterns de RF-15) |
| **Toxicity classification** | L2 | ML ONNX para detectar conteudo toxico ou prejudicial |
| **Jailbreak detection** | L2 | ML ONNX para detectar tentativas sofisticadas de jailbreak |
| **Semantic adversarial analysis** | L3 | LLM Judge avalia semanticamente se request/response e segura |
| **Audit emission** | Todos | Emitir GuardrailsEvent estruturado com contexto completo de tenant |

## 3. Contratos API (minimo)

**Bloqueio L1 — payload size:**
- HTTP 413
- Body: `{"error":{"type":"payload_too_large","message":"Request payload exceeds tenant limit"},"tenant_id":"acme","limit_bytes":524288}`

**Bloqueio L1 — model denied:**
- HTTP 403
- Body: `{"error":{"type":"model_denied","message":"Model not allowed for this tenant"},"tenant_id":"acme","app_id":"payments-app","model":"gpt-4-unrestricted"}`

**Bloqueio L1 — rate limit:**
- HTTP 429 + header `Retry-After: 15`
- Body: `{"error":{"type":"rate_limit_exceeded","message":"Too many requests for this tenant"},"tenant_id":"acme","client_id":"sk-dev-team"}`

**Bloqueio L1 — pattern match:**
- HTTP 400
- Body: `{"error":{"type":"security_violation","message":"Request flagged by security rules"},"layer":"L1","score":0.78}`

**Bloqueio L2 — ML classifier:**
- HTTP 400
- Body: `{"error":{"type":"content_policy_violation","message":"Content blocked by security classifier"},"layer":"L2","score":0.87,"category":"jailbreak"}`

**Warning sem bloqueio:**
- Header: `X-Guardrails-Warning: L2 jailbreak_suspected (score: 0.65, tenant: acme)`
- Response entregue normalmente ao cliente

**Endpoint admin:**
- `GET /admin/guardrails/stats` — JSON com breakdown por tenant e por camada (ver RF-32-FUNCIONAL.md)

## 4. Fluxos detalhados

### Fluxo 1: Request bloqueada em L1 — rate limit

1. Request chega em `/v1/chat/completions`.
2. Gateway popula `ctx.metadata["tenant_id"]="acme"`, `ctx.metadata["app_id"]="payments-app"`.
3. `GuardrailsPlugin.before_request`: resolve PolicyBundle para chave `"acme:payments-app"`.
4. L1 — payload size: OK (380 KB < 512 KB). L1 — model: OK. L1 — rate bucket `"acme:sk-dev-team"`: vazio → block.
5. Emite `GuardrailsEvent {layer:"L1", action:"block", reason:"rate_limit", tenant_id:"acme", app_id:"payments-app", client_id:"sk-dev-team", score:1.0}`.
6. Retorna `PluginResult::Block` com HTTP 429 e header `Retry-After: 15`.
7. Request não é enviada ao LLM.

### Fluxo 2: Request bloqueada em L2 — jailbreak ML

1. L1 passa: payload OK, modelo permitido, rate OK, score L1 = 0.21 < 0.7.
2. L2 habilitado e `random() < 0.5` (sample_rate) → executa classificador ONNX.
3. Classifica prompt: `toxicity_score=0.12`, `jailbreak_score=0.91`.
4. `score = max(0.12, 0.91 * 0.9) = 0.819 >= jailbreak_threshold (0.75)` → block.
5. Emite evento L2 com score, categoria "jailbreak" e triplete de tenant.
6. Retorna `PluginResult::Block` com HTTP 400 `content_policy_violation`.

### Fluxo 3: L3 assíncrono — sampling

1. L1 e L2 passam. Request enviada ao LLM. Response recebida.
2. `after_response` executa L1 response check (patterns na response do LLM).
3. L3 habilitado e `random() < 0.05` (sample_rate) → agenda tarefa async.
4. Response já enviada ao cliente (sem latência adicional de L3).
5. Async: LLM Judge analisa `{prompt, response}` com system prompt de avaliação de segurança.
6. Judge retorna `{"verdict":"safe","score":0.08}`.
7. `GuardrailsEvent` emitido com `{layer:"L3", verdict:"safe", score:0.08, tenant_id:"acme"}` → audit sink.

### Fluxo 4: trusted_client_id — bypass de patterns

1. Request com `key_alias = "internal-admin"` (em `trusted_client_ids`).
2. L1 payload size check: sempre executa (sem bypass).
3. L1 model check: executa (trusted não bypassa allow/deny de modelo).
4. L1 rate limit: trusted_client_ids **não** bypassam rate limit por design (evitar abuso interno).
5. L1 pattern matching: **bypassa** para trusted. L2: **bypassa** para trusted.
6. Request enviada ao LLM normalmente.

## 5. Validacoes e guardrails de implementacao

- **Isolamento de tenant:** `resolve_bundle` usa chave composta `"tenant_id:app_id"` — teste unitário de isolamento obrigatório; never cross-pollinate.
- **Fallback seguro:** PolicyBundle `"default"` deve ser configurado como o **mais restritivo**; tenants específicos podem afrouxar limites, nunca endurecer além do default.
- **L2 sample_rate:** Validar em `init()` que `0.0 ≤ sample_rate ≤ 1.0`; `sample_rate=0.0` desabilita L2 efetivamente.
- **L3 fire-and-forget:** Exceção no L3 é capturada em `try/catch` e logada; nunca propaga para o handler principal do gateway.
- **trusted_client_ids:** Não bypassa payload size check nem verificação de modelo — somente pattern matching e L2.
- **Rate bucket thread-safety:** Token bucket por `"tenant:client_id"` protegido por `shared_mutex`; write lock apenas em decrement.

## 6. Seguranca e autorizacao

- Endpoint `/admin/guardrails/stats` requer `ADMIN_KEY`; sem autenticação retorna 401.
- PolicyBundle **nunca** é exposta via endpoint (revelaria regras de segurança por tenant).
- Logs de bloqueio L2/L3 não incluem conteúdo completo do prompt — apenas score, categoria e metadata de identidade.
- `trusted_client_ids` deve ser gerenciada com controle de acesso; representa bypass parcial de segurança.
- Evento L3 no audit sink inclui apenas metadata — não persiste conteúdo do prompt/response.

## 7. Telemetria e observabilidade

### Eventos recomendados

- `guardrails_l1_blocked` (reason, tenant_id, app_id, client_id, model, score)
- `guardrails_l1_warned` (reason, tenant_id, app_id, client_id, score)
- `guardrails_l2_classified` (category, score, verdict, tenant_id, app_id, sampled_at)
- `guardrails_l3_verdict` (verdict, score, judge_model, tenant_id, app_id, latency_ms)
- `guardrails_policy_resolved` (tenant_id, app_id, bundle_key, is_default)

### Metadata minima por evento

| Campo | Descricao |
|-------|-----------|
| `request_id` | X-Request-Id da request |
| `tenant_id` | ctx.metadata["tenant_id"] |
| `app_id` | ctx.metadata["app_id"] |
| `client_id` | ctx.key_alias |
| `layer` | "L1" / "L2" / "L3" |
| `action` | "block" / "warn" / "pass" |
| `score` | Score da camada (0.0–1.0) |
| `reason` | payload_size / model_denied / rate_limit / pattern_match / toxicity / jailbreak / judge |
| `model` | Modelo solicitado |

## 8. Criterios de aceitacao (PoC/MVP)

- [ ] L1 bloqueia payload > limite com 413; log inclui triplete de tenant.
- [ ] L1 bloqueia modelo negado com 403 usando PolicyBundle correto por tenant+app.
- [ ] L1 rate limit por tenant:client_id retorna 429 com Retry-After.
- [ ] L1 pattern matching bypassa para trusted_client_ids.
- [ ] L2 executa apenas quando habilitado e conforme sample_rate do tenant.
- [ ] L2 bloqueia com score >= threshold e emite evento com categoria e score.
- [ ] L3 executa assincronamente sem impactar latência da response ao cliente.
- [ ] Exceção em L3 é capturada e logada — nunca retorna erro ao cliente.
- [ ] Todos os eventos incluem tenant_id, app_id, client_id.
- [ ] PolicyBundle "default" aplicado quando tenant sem política configurada.

## Checklist de qualidade da especificacao AI

- [x] Intents mapeadas para fluxos reais
- [x] Guardrails e RBAC explicitos
- [x] Contratos API minimos definidos
- [x] Eventos e metadata de observabilidade definidos
- [x] Criterios de aceitacao objetivos e testaveis
