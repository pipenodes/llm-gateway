# ADR-01 — Core vs Plugin: Decisoes Arquiteturais

- **Data:** 2026-03-09
- **Status:** Aceito
- **Contexto:** Alinhamento ao modelo AllTrue (AI TRiSM) para definicao do que permanece no core do HERMES e o que vira plugin. Garantir consistencia com arquitetura atual: core minimo + servicos plugaveis + pipeline de plugins. Os pilares TRiSM (Transparency, Risk, Security, Management) orientam a distribuicao de capacidades entre core e plugins.

## Decisao

O core mantem apenas o necessario ao fluxo minimo de toda request e infraestrutura essencial (Config, Logger, PluginManager, interfaces IAuth, IRateLimiter, ICache, IRequestQueue, IMetricsCollector). Tudo que e opcional por deployment, tem multiplas implementacoes ou executa fora do path critico vira plugin. O core expoe pontos de extensao (ex.: IAuditSink) com implementacao no-op por default; plugins implementam persistencia, relatorios e alertas.

## Criterios de Decisao

### Fica no CORE quando

- E **necessario ao fluxo minimo** de toda request (ex.: decidir se a request segue ou e rejeitada antes de chegar ao backend).
- E **infraestrutura** que o gateway nao funciona sem: Config, Logger, PluginManager, interfaces de servico (ICache, IAuth, IRateLimiter, IRequestQueue, IMetricsCollector).
- O gateway precisa de um **contrato estavel** (interface) mesmo quando a implementacao e "null" (build edge): ex. IAuth, IRateLimiter.
- Expoe **endpoints ou contratos** que outros sistemas (ex.: AllTrue, SIEM) consomem de forma padrao e o gateway deve oferecer mesmo com build minimo (ex.: health, metricas minimas, lista de modelos).

### Vira PLUGIN quando

- Comportamento **opcional** por deployment (ativado/desativado por config).
- **Multiplas implementacoes** ou backends possiveis (ex.: audit em arquivo vs HTTP vs nenhum).
- Nao e necessario para o gateway **responder** a request; apenas observa, transforma ou reage (ex.: logging, alerting, relatorios).
- Logica de **politica** ou negocio que varia por cliente (moderacao, PII, compliance por regulamento).
- Execucao **fora do path critico** da request (ex.: scan de seguranca agendado, geracao de relatorio).

## Decisoes por Pilar

### Visibility

| Item | Decisao | Justificativa |
|------|---------|---------------|
| Listagem de modelos, keys, plugins, metricas | **Core (ja e)** | Endpoints `/v1/models`, `/admin/keys`, `/admin/plugins`, `/metrics` fazem parte do contrato minimo do gateway; edge pode expor lista minima de modelos. |
| Inventario/registro de "ativos" (modelos, backends) consultavel | **Core: interface opcional** | Definir interface leve no core (ex. `IAssetRegistry`) so se for necessario para outros plugins ou admin; implementacao "vazia" no edge. **Plugin** opcional que popula o registro a partir de config ou descoberta. |
| Discovery de shadow AI / scan de ambiente | **Plugin (ou ferramenta externa)** | Fora do escopo do gateway como proxy; pode existir um plugin que reporta "o que este gateway conhece" para um sistema externo (ex. AllTrue). |

**Resumo:** Visibilidade do que o gateway ja conhece fica no core (endpoints atuais). Qualquer "discovery" alem do proprio processo e plugin ou ferramenta externa.

### ASPM

| Item | Decisao | Justificativa |
|------|---------|---------------|
| Config centralizada (config.json, env) | **Core (ja e)** | Config e infraestrutura do core. |
| Relatorio de misconfig / vulnerabilidades | **Plugin** | Geracao de relatorio e opcional, pode rodar sob demanda ou agendado; nao afeta o path da request. Um plugin pode expor `GET /admin/posture` ou similar e agregar config + estado dos plugins. |
| Inventario de agentes/chatbots escaneaveis | **Plugin** | Se no futuro o gateway modelar "agentes", sera extensao opcional (plugin) que mantem inventario e pode ser consumido por ferramentas ASPM externas. |

**Resumo:** ASPM como "scan e relatorio" e plugin; configuracao e admin basico continuam no core.

### Runtime Protection

| Item | Decisao | Justificativa |
|------|---------|---------------|
| Auth (API keys, IP whitelist) | **Core: interface IAuth** (ja e) | Decisao de aceitar ou rejeitar a request e fundamental; implementacao como plugin (AuthPlugin) ou NullAuth. |
| Rate limiting | **Core: interface IRateLimiter** (ja e) | Idem. |
| Payload max, CORS | **Core (ja e)** | Aplicado antes do pipeline. |
| Prompt injection, content moderation, PII, response validator | **Plugin (ja sao)** | Politicas opcionais; cada deployment escolhe quais ativar no pipeline. |
| Politicas declarativas de dados (ex. "bloquear se PII") | **Plugin** | Implementacao como plugin de pipeline que avalia politicas (pode ser o proprio pii_redactor + response_validator ou um plugin "policy_engine" futuro). |

**Resumo:** Runtime protection mantem auth e rate limit como servicos do core (interfaces); demais protecoes continuam e evoluem como plugins de pipeline.

### Detection & Response

| Item | Decisao | Justificativa |
|------|---------|---------------|
| Request ID (X-Request-Id), contexto por request | **Core (ja e)** | Necessario para rastreabilidade minima e logs; nao opcional. |
| Logging estruturado (stdout) | **Core ou plugin leve** | Pode permanecer no core como log minimo por request; log enriquecido (corpos, respostas) como plugin `logging`. |
| **Audit log** (persistencia JSONL, rotacao, consulta admin) | **Plugin** | Opcional por deployment; multiplos backends possiveis (arquivo, HTTP, S3). Core expoe apenas um **ponto de extensao** (ex. evento "request_completed" ou interface `IAuditSink`) chamado apos cada request; implementacao default "no-op". Plugin `audit` implementa persistencia e `GET /admin/audit`. Build edge sem plugin de audit nao persiste nada. |
| Metricas (contadores, Prometheus) | **Core: interface IMetricsCollector** (ja e) | Metricas minimas no core; full/Prometheus como implementacao plugavel. |
| **Alertas** (regras de risco, notificacoes) | **Plugin** | Plugin que consome eventos (do audit sink ou do pipeline), avalia regras e envia notificacoes (webhook, etc.). Nao no core. |
| Resposta automatizada (bloquear key, escalar) | **Plugin** | Plugin que reage a eventos (ex. apos N bloqueios por prompt injection) e chama admin API para revogar key ou disparar webhook. |

**Resumo:** Audit vira **plugin** (ou servico implementando interface opcional do core); alerting e resposta automatizada sao plugins que consomem eventos/audit.

### Security Testing

| Item | Decisao | Justificativa |
|------|---------|---------------|
| Protecao em runtime (bloquear prompt injection) | **Plugin (ja e)** | prompt_injection no pipeline. |
| **Scan proativo** (prompt injection, jailbreak) | **Plugin ou ferramenta externa** | Nao faz parte do path da request; roda sob demanda ou agendado. Plugin que adiciona endpoint `POST /admin/security-scan` (ou ferramenta CLI) e gera relatorio. |
| Relatorio de vulnerabilidades de modelos/agentes | **Plugin** | Mesmo plugin de ASPM/security testing ou plugin separado que gera relatorio consumivel por ferramentas externas. |

**Resumo:** Security testing "ativo" (scan + relatorio) e plugin ou binario externo; protecao em tempo real ja e plugin.

### Compliance

| Item | Decisao | Justificativa |
|------|---------|---------------|
| Dados para compliance (request_id, key, modelo, tokens, etc.) | **Core + Plugin** | Core (ou plugin audit) garante que os **dados** existam (audit log); plugin **compliance** ou **reporting** agrega e exporta em formatos prontos (EU AI Act, frameworks). |
| Relatorios prontos por regulamento | **Plugin** | Plugin que le audit (ou eventos) e gera relatorios (PDF, JSON, etc.) por framework. Nao no core. |
| Mapeamento controle <-> requisito | **Plugin ou documentacao** | Pode ser plugin que anota/exporta metadados de controles; ou apenas documentacao (ex. COMPARISON_HERMES_VS_ALLTRUE.md + este doc). |

**Resumo:** Core (e plugin de audit) fornecem os dados; relatorios e formatos de compliance sao plugin.

## Tabela Resumo

| Capacidade (AllTrue) | Core | Plugin |
|----------------------|------|--------|
| Visibility: listar modelos/keys/plugins/metricas | Endpoints atuais (ja) | — |
| Visibility: inventario/registry opcional | Interface opcional (se necessario) | Plugin que popula/exporta |
| ASPM: relatorio misconfig/vulns | — | Plugin (posture/report) |
| Runtime: auth, rate limit | Interfaces IAuth, IRateLimiter (ja) | AuthPlugin, RateLimitPlugin |
| Runtime: prompt injection, PII, moderacao, validator | — | Ja sao plugins |
| Detection: request ID, contexto | Core (ja) | — |
| Detection: audit log persistido | Interface/evento opcional (sink) | Plugin `audit` (JSONL, admin, backends) |
| Detection: alertas | — | Plugin `alerting` |
| Response: bloquear key, escalar | — | Plugin que reage a eventos |
| Security testing: scan ativo | — | Plugin ou ferramenta |
| Compliance: dados | Via audit / eventos | Plugin audit fornece |
| Compliance: relatorios prontos | — | Plugin `compliance_report` |

## Implicacoes

- **Audit Log (RF-04):** A spec deve especificar que a **persistencia e o endpoint** `GET /admin/audit` sao implementados por um **plugin** `audit` (ou `audit_log`), e que o core oferece apenas um **ponto de extensao** (ex. `IAuditSink` ou callback "on_request_completed") com implementacao default no-op. Isso permite builds sem audit e multiplos backends (arquivo, HTTP, etc.) via config do plugin.
- **Introducao de IAuditSink:** O core deve expor interface `IAuditSink` (ou equivalente) com implementacao no-op por default; o plugin `audit` implementa a persistencia real.
- **Usage Tracking (RF-02):** Pode permanecer como servico com interface no core (ex. `IUsageTracker`) se for usado para decisao de quota no path da request; implementacao como plugin. Caso seja so contabilidade pos-request, pode ser apenas plugin que consome eventos.
- **Proximos passos:** Ajustar spec RF-04 para modelo "core: evento/sink; plugin: persistencia + admin"; criar specs para plugins `audit`, `alerting`, opcionalmente `posture` e `compliance_report`; documentar na ARCHITECTURE.md o desenho core vs plugin para Detection & Response e Compliance.

## Adendo: Ordem Mandatoria do Security Pipeline Enterprise

**Data:** 2026-03-23 | **Status:** Aceito

Com a introducao dos plugins enterprise `guardrails` (RF-32), `data_discovery` (RF-34), `dlp` (RF-33) e `finops` (RF-35), fica definida a seguinte ordem mandatoria no pipeline de plugins quando esses quatro estiverem ativos:

```
guardrails → data_discovery → dlp → [providers] → finops (after_response)
```

### Justificativas de ordem

| Posicao | Plugin | Por que nao pode ser reordenado |
|---------|--------|----------------------------------|
| 1 | `guardrails` L1 | Bloqueia requests invalidos (payload, modelo, rate limit, regex) antes de qualquer processamento custoso. Sincronico, <10ms. |
| 2 | `data_discovery` | Classifica conteudo e popula `ctx.metadata["discovered_tags"]`. O `dlp` depende desse campo para aplicar acoes corretas. |
| 3 | `dlp` | Le `discovered_tags` para aplicar block/redact/quarantine. Nao pode rodar antes do discovery, pois nao teria as tags de sensibilidade. |
| — | `guardrails` L2/L3 | L2 (ML classifier) pode rodar apos L1 no mesmo hook; L3 (LLM Judge) e sempre assincrono e nao bloqueia o response. |
| after_response | `finops` | Contabiliza custo real pos-resposta (precisa dos tokens usados retornados pelo backend). Nao pode rodar antes do provider. |

### Contexto multi-tenant obrigatorio

Todos os quatro plugins requerem que o gateway core popule `ctx.metadata["tenant_id"]`, `ctx.metadata["app_id"]` e `ctx.metadata["client_id"]` **antes** de iniciar o pipeline. Mecanismo documentado em `docs/specs/RF-10-ADENDO-TENANT-CTX.md`.

### Regra de dependencia no PluginManager

O `PluginManager` deve validar, em `init()`, que quando `dlp` esta ativo, `data_discovery` tambem esta ativo e aparece antes na lista `pipeline[]`. Falha de validacao deve impedir o boot do gateway com mensagem clara:

```
FATAL: plugin 'dlp' requires 'data_discovery' to be enabled and listed before it in pipeline[].
```

## Referencias

- docs/ARCHITECTURE.md
- docs/COMPARISON_HERMES_VS_ALLTRUE.md
- RF-04 (Audit Log), RF-10 (Plugin System)
- RF-04-FUNCIONAL.md (Audit Log), RF-10-FUNCIONAL.md (Plugin System)
- docs/specs/RF-10-ADENDO-TENANT-CTX.md (contexto multi-tenant no core)
- docs/specs/RF-32-FUNCIONAL.md (GuardRails), RF-33-FUNCIONAL.md (DLP), RF-34-FUNCIONAL.md (Data Discovery), RF-35-FUNCIONAL.md (FinOps)
