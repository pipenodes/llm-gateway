# RF-06-UX — Dashboard Web

- **RF:** RF-06
- **Titulo:** Dashboard Web
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** RASCUNHO

**Referencia funcional:** RF-06-FUNCIONAL.md

## 1. Resumo executivo

SPA embutido no gateway para administracao e monitoramento em tempo real. Protegido pela admin key. Servido como HTML/JS/CSS estatico sem framework. Consome endpoints existentes (/metrics, /admin/keys, /health, etc.) com polling a cada 5s. Vanilla JS + Chart.js via CDN para graficos.

## 2. Personas e cenarios principais

### Personas

- **Administrador de infraestrutura:** monitora saude, cria/revoga keys, verifica metricas
- **Desenvolvedor:** verifica requests recentes, debugga problemas, consulta audit log

### Cenarios

- Monitoramento rapido de saude e metricas
- Gestao de API keys (criar, revogar, listar)
- Consulta de requests recentes e audit log
- Visualizacao de cache hit rate e memoria

## 3. Principios de design

- **Simplicidade:** vanilla JS, sem framework, zero dependencias de build
- **Responsividade:** funcionar em desktop e mobile
- **Performance:** polling a cada 5s, sem WebSocket
- **Seguranca:** admin key em sessionStorage, mascarada na UI

## 4. Regras de interacao e microcopy

- **Login:** input de admin key com placeholder "Enter admin key"
- **Erro de auth:** "Invalid admin key. Please try again."
- **Sucesso ao criar key:** "Key created successfully. Copy the key now — it won't be shown again."
- **Confirmacao ao revogar:** "Are you sure you want to revoke key '{alias}'? This action cannot be undone."
- **Estado vazio (sem keys):** "No API keys yet. Create your first key to get started."
- **Estado loading:** indicador visual (spinner ou skeleton) durante fetch
- **Erro de rede:** "Failed to load data. Check your connection and try again."

## 5. Acessibilidade e internacionalizacao

- Contraste WCAG AA minimo
- Labels em todos os inputs
- Interface em ingles (lingua padrao da API)
- Suporte a navegacao por teclado
- Status indicator com cor + texto (nao apenas cor)

## 6. Spec de telas (wireframes textuais)

### Tela 1: Login

- Input de admin key (type password, mascarado)
- Botao "Connect"
- Validacao contra GET /health com auth header
- Erro exibido abaixo do input em caso de falha

### Tela 2: Overview

- **Cards:** Uptime, Requests Total, Active Now, Memory RSS/Peak
- **Graficos:** Requests/s (linha temporal), Memory (linha temporal), Cache Hit Rate (donut)
- **Status indicator:** verde (healthy) / vermelho (unhealthy) baseado em /health
- Nav com links para Overview, API Keys, Recent Activity, Cache

### Tela 3: API Keys

- **Tabela:** alias, prefix, created, last_used, request_count, rate_limit
- Botao "Create Key" -> modal com: alias, rate_limit_rpm, ip_whitelist
- Botao "Revoke" com confirmacao
- Estado vazio: mensagem + CTA "Create Key"

### Tela 4: Recent Activity

- **Tabela:** timestamp, request_id, key_alias, model, status, latency_ms
- **Filtros:** key, model, status, periodo
- Requer plugin audit habilitado
- Estado vazio (audit desabilitado): "Recent Activity requires the Audit Log plugin. Enable it in config to see requests."

### Tela 5: Cache

- **Cards:** Entries, Memory Used, Hit Rate, Evictions
- **Cache semantico:** entries, threshold, hit rate (se habilitado)
- Botao "Clear Cache" (futuro)

## 7. Mapeamento de componentes

- Chart.js via CDN para graficos (linha, donut)
- Tabelas HTML nativas com sorting basico
- Modais com overlay para criar key
- Status badge (verde/amarelo/vermelho)
- Cards com titulo, valor grande, subtitulo
- Nav lateral ou horizontal para secao

## 8. Criterios de aceitacao (UI / QA)

- [ ] Login com admin key funciona e persiste em sessionStorage
- [ ] Overview mostra metricas atualizadas a cada 5s
- [ ] Graficos renderizam corretamente com Chart.js
- [ ] CRUD de API keys funciona (criar, listar, revogar)
- [ ] Recent Activity mostra ultimas requests quando audit habilitado
- [ ] Dashboard e responsivo (desktop + mobile)
- [ ] Dashboard funciona sem Chart.js CDN (graficos nao renderizam mas resto funciona)
- [ ] Estados vazios e de erro exibem mensagens adequadas
- [ ] Confirmacao antes de revogar key

## 9. Observacoes tecnicas

- HTML/JS/CSS embutido em constexpr string_view no binario C++ (padrao de openapi_spec.h)
- Aumento estimado de 50-100KB no binario
- Nao requer build step separado
- CORS nao e problema (mesmo host)
- Em ambientes air-gapped, Chart.js CDN pode falhar; graficos nao renderizam mas demais funcionalidades operam

## Checklist de qualidade da especificacao UX

- [x] Fluxos principais cobertos
- [x] Estados vazios, loading e erro definidos
- [x] Microcopy essencial documentada
- [x] Requisitos de acessibilidade e i18n explicitos
- [x] Criterios de UI/QA objetivos
