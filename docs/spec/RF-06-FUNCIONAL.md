# RF-06 — Dashboard Web

- **RF:** RF-06
- **Titulo:** Dashboard Web
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Dashboard web embutido no proprio gateway (sem servidor separado) para administracao e monitoramento em tempo real. Servido como HTML/JS/CSS estatico pelo gateway, consumindo os endpoints de metricas e admin existentes. Protegido pela admin key.

## Escopo

- **Inclui:** SPA embutido, vanilla JS, Chart.js (via CDN), metricas/keys/audit visualization, rota GET /dashboard
- **Nao inclui:** Framework React/Vue, servidor separado, autenticacao OAuth

## Descricao Funcional Detalhada

### Arquitetura

Browser -> GET /dashboard -> HERMES -> HTML/JS/CSS (SPA). SPA faz fetch para /metrics, /admin/keys, /admin/usage, /admin/audit, /health. Todas as chamadas API exigem admin key (Authorization: Bearer).

### Componentes

- **dashboard.h:** HTML/JS/CSS embutido como constexpr string_view (mesmo padrao de openapi_spec.h).
- **SPA:** Single Page Application com vanilla JS (sem framework), Chart.js via CDN para graficos.

### Cenarios

1. **Monitoramento rapido:** Abrir browser e ver requests/s, latencia, uso de memoria em tempo real.
2. **Gestao de keys:** Criar, revogar e listar API keys sem curl.
3. **Debugging:** Ver requests recentes, erros, metricas de cache sem acessar servidor.
4. **Onboarding:** Novos membros exploram a API pelo dashboard.

### Secoes do Dashboard

1. **Overview Cards:** Uptime, Requests Total, Active Now, Memory (RSS/Peak MB)
2. **Graficos (Chart.js):** Requests/s (linha temporal), Memoria (RSS vs Peak), Cache Hit Rate (gauge/donut)
3. **API Keys:** Tabela (alias, prefix, created, last_used, request_count, rate_limit, status), botao Create Key, botao Revoke com confirmacao
4. **Cache:** Entries, Memory, Hit Rate, Evictions; botao Clear Cache (futuro)
5. **Recent Activity:** Ultimas N requests (requer Audit Log RF-04)

## Interface / Contrato

```cpp
// dashboard.h -- similar a openapi_spec.h
#pragma once
#include <string_view>

static constexpr std::string_view DASHBOARD_HTML = R"html(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>HERMES Dashboard</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js@4/dist/chart.umd.min.js"></script>
</head>
<body>
    <nav><h1>HERMES</h1><div id="status-indicator"></div></nav>
    <main>
        <section id="overview"><!-- Cards: Uptime, Requests Total, Active, Memory --></section>
        <section id="charts"><!-- Requests/s chart, Memory chart --></section>
        <section id="keys"><!-- API Keys table, Create/Revoke buttons --></section>
        <section id="recent"><!-- Recent requests from audit log --></section>
    </main>
    <script>
        // Auto-refresh metrics every 5s
        // Admin key em sessionStorage para Authorization: Bearer
    </script>
</body>
</html>
)html";
```

O dashboard pede a admin key ao abrir (input field) e armazena em sessionStorage para enviar como Authorization: Bearer em todas as chamadas fetch.

## Configuracao

### config.json

```json
{
  "dashboard": {
    "enabled": true,
    "refresh_interval_seconds": 5
  }
}
```

### Variaveis de Ambiente

| Variavel | Descricao | Default |
|----------|-----------|---------|
| `DASHBOARD_ENABLED` | Habilitar dashboard web | `false` |

## Endpoints

| Metodo | Path | Descricao |
|--------|------|-----------|
| `GET` | `/dashboard` | Pagina principal do dashboard |

O dashboard e publico (nao requer auth para carregar o HTML), mas todas as chamadas de API internas exigem admin key.

## Regras de Negocio

- Dashboard servido pelo mesmo host do gateway — sem problemas de CORS.
- Admin key em sessionStorage: risco se alguem tiver acesso ao browser. Mitigacao: expirar apos inatividade (futuro).
- Chart.js via CDN: em ambientes air-gapped, graficos nao funcionam. Alternativa: embutir Chart.js minificado.

## Dependencias e Integracoes

- **Internas:** Endpoints existentes (/metrics, /admin/keys, /health)
- **Externas:** Chart.js via CDN (opcional; graficos funcionam sem ele)
- **Opcionais:** RF-04 (Audit Log) para secao Recent Activity, RF-02 (Usage Tracking) para secao de quotas

## Criterios de Aceitacao

- [ ] GET /dashboard retorna HTML do SPA
- [ ] Dashboard exibe status do gateway (health)
- [ ] Overview cards: Uptime, Requests Total, Active, Memory
- [ ] Graficos de requests/s e memoria com polling (ex: 5s)
- [ ] Tabela de API keys com acoes Create e Revoke
- [ ] Admin key solicitada ao abrir; armazenada em sessionStorage
- [ ] Todas as chamadas fetch usam Authorization: Bearer
- [ ] Secao Recent Activity quando Audit Log habilitado
- [ ] Responsivo para mobile

## Riscos e Trade-offs

1. **Tamanho do binario:** HTML/JS/CSS embutido aumenta o binario (50-100KB extra).
2. **CDN dependency:** Chart.js via CDN requer internet. Air-gapped: graficos nao funcionam. Alternativa: embutir Chart.js minificado.
3. **Seguranca:** Admin key em sessionStorage. Alguem com acesso ao browser pode extrai-la.
4. **Responsividade:** Dashboard precisa funcionar em mobile.
5. **Sem framework:** Vanilla JS pode ficar complexo se dashboard crescer. Manter simples.

## Status de Implementacao

IMPLEMENTADO — Dashboard SPA embutido em dashboard.h como constexpr string_view, servido em GET /dashboard. Vanilla JS + Chart.js (CDN). Secoes: overview cards (uptime, requests, active, cache), graficos (requests/min, latency), API keys (create/revoke), usage, audit activity, plugins. Admin key via sessionStorage. Auto-refresh a cada 5s. Memory card (RSS/Peak MB) adicionado ao dashboard usando dados do endpoint /metrics (memory_rss_kb, memory_peak_kb). Uptime agora obtido do servidor via /metrics (uptime_seconds) em vez de timer local.

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
