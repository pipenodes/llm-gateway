# RF-29 — Compliance Report

- **RF:** RF-29
- **Titulo:** Compliance Report
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Plugin que gera relatorios de uso/auditoria agregados a partir dos dados do audit log (ficheiros JSONL ou API interna do plugin audit). Expoe GET /admin/compliance/report com parametros (from, to, format, agrupamento). Formato JSON ou CSV. Futuramente templates por regulamento (ex.: EU AI Act).

## Escopo

- **Inclui:** GET /admin/compliance/report; query params from, to, format (json|csv), group_by (key_alias|model|day); agregacao de uso (tokens, requests, latencia media); IComplianceReportProvider.
- **Nao inclui:** Templates por regulamento (EU AI Act); scan ativo de compliance.

## Descricao Funcional Detalhada

### Comportamento

- **GET /admin/compliance/report:** Requer ADMIN_KEY.
- Query params: `from` (timestamp), `to` (timestamp), `format` (json|csv), opcionalmente `group_by` (key_alias|model|day).
- Resposta: Agregacao de uso (totais de tokens, requests, latencia media) no periodo; opcionalmente por key ou por modelo.
- CSV: cabecalho + linhas.

### Implementacao

- Plugin implementa IComplianceReportProvider com metodo report(from, to, format) -> corpo + Content-Type.
- Gateway expoe GET /admin/compliance/report e delega ao provider (primeiro plugin que implementa).
- Se nao houver plugin: 404.
- Leitura: plugin audit expoe metodo ou compliance le ficheiros JSONL (path configurado).

## Interface / Contrato

- Interface: IComplianceReportProvider
- Metodo: report(from, to, format) -> (body, Content-Type)
- Fonte de dados: audit log (JSONL ou API do plugin audit)

## Configuracao

Path do audit log compartilhado com plugin audit; ou config especifica do compliance.

## Endpoints

| Metodo | Path | Auth | Query Params | Descricao |
|---|---|---|---|---|
| GET | /admin/compliance/report | ADMIN_KEY | from, to, format, group_by | Relatorio de compliance agregado |

### Exemplo de Resposta (JSON)

```json
{
  "period": { "from": 1740355200, "to": 1740441600 },
  "totals": { "requests": 1500, "tokens": 450000, "avg_latency_ms": 1200 },
  "by_key": { "dev-team": { "requests": 800, "tokens": 240000 } },
  "by_model": { "llama3:8b": { "requests": 1200 } }
}
```

## Regras de Negocio

1. ADMIN_KEY obrigatorio.
2. from/to em timestamp Unix.
3. format: json ou csv.
4. group_by opcional: key_alias, model, day.
5. Agregacao a partir de dados de audit existentes.

## Dependencias e Integracoes

- **Internas:** Plugin audit (dados), Feature 10 (Plugin System).
- **Externas:** Nenhuma.

## Criterios de Aceitacao

- [ ] GET /admin/compliance/report com from, to, format
- [ ] Resposta JSON com agregacao
- [ ] Resposta CSV com cabecalho
- [ ] group_by key_alias, model, day
- [ ] ADMIN_KEY requerido
- [ ] 404 quando sem plugin provider

## Riscos e Trade-offs

1. **Volume de dados:** Periodos longos podem gerar relatorios grandes; considerar paginacao.
2. **Acoplamento:** Preferir leitura de ficheiros em path configurado; evitar acoplamento direto com audit.

## Status de Implementacao

IMPLEMENTADO — GET /admin/compliance/report, agregacao de audit, JSON/CSV, group_by.

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
