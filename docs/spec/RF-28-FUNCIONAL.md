# RF-28 — Posture ASPM

- **RF:** RF-28
- **Titulo:** Posture ASPM
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Plugin que expoe um endpoint admin GET /admin/posture (ou GET /admin/security-posture) para relatorio de postura de seguranca do gateway. Agrega config exposta (sem secrets), lista de plugins ativos, modelos conhecidos e opcionalmente resultados de checagens (ex.: admin sem IP whitelist). Retorna JSON para integracao com ferramentas ASPM externas (ex.: AllTrue). Nao faz scan de rede; apenas relatorio do estado do gateway.

## Escopo

- **Inclui:** GET /admin/posture; config_snapshot (sem secrets); plugins ativos; modelos (alias -> modelo); checks opcionais (admin_ip_whitelist, etc); autenticacao ADMIN_KEY.
- **Nao inclui:** Scan de rede; execucao de testes de seguranca ativos.

## Descricao Funcional Detalhada

### Comportamento

- **GET /admin/posture:** Requer ADMIN_KEY. Resposta JSON com:
  - **config_snapshot:** Config relevante (porta, cache_enabled, rate_limit, queue_enabled, etc.) sem chaves ou secrets.
  - **plugins:** Lista de plugins ativos (nome, versao).
  - **models:** Modelos expostos (alias -> modelo resolvido) se disponivel.
  - **checks:** Array de checagens opcionais (ex.: `{ "id": "admin_ip_whitelist", "passed": false, "message": "Admin IP whitelist empty" }`).

### Implementacao

- Gateway expoe GET /admin/posture e monta payload (config sem secrets, list_plugins, modelos).
- Plugin opcional posture pode adicionar checks via IPostureProvider (get_checks()).
- Endpoint existe mesmo sem plugin; com plugin, enriquece com checks.

## Interface / Contrato

- Endpoint: GET /admin/posture
- Auth: ADMIN_KEY (header ou query)
- Response: JSON com config_snapshot, plugins, models, checks

```json
{
  "config_snapshot": { "port": 8080, "cache_enabled": true, "rate_limit": 60 },
  "plugins": [{ "name": "rate_limiter", "version": "1.0.0" }],
  "models": { "llama3": "llama3:8b" },
  "checks": [{ "id": "admin_ip_whitelist", "passed": false, "message": "..." }]
}
```

## Configuracao

N/A — endpoint admin; config vem do gateway.

## Endpoints

| Metodo | Path | Auth | Descricao |
|---|---|---|---|
| GET | /admin/posture | ADMIN_KEY | Retorna postura de seguranca em JSON |

## Regras de Negocio

1. ADMIN_KEY obrigatorio.
2. Nenhum secret em config_snapshot.
3. Checks opcionais via IPostureProvider.
4. Nao executa scan; apenas estado atual.

## Dependencias e Integracoes

- **Internas:** Feature 10 (Plugin System); config do gateway; plugin_manager.
- **Externas:** Ferramentas ASPM (AllTrue, etc).

## Criterios de Aceitacao

- [ ] GET /admin/posture retorna JSON
- [ ] config_snapshot sem secrets
- [ ] Lista de plugins ativos
- [ ] Lista de modelos (alias -> modelo)
- [ ] Checks opcionais via plugin
- [ ] ADMIN_KEY requerido

## Riscos e Trade-offs

1. **Exposicao de config:** Garantir que nenhum secret vaze.
2. **Checks plugaveis:** Interface IPostureProvider para extensibilidade.

## Status de Implementacao

IMPLEMENTADO — GET /admin/posture, config snapshot, plugin status, model list, security checks.

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
