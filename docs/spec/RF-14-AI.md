# RF-14-AI — Content Moderation

- **RF:** RF-14
- **Titulo:** Content Moderation
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

**Referencia funcional:** RF-14-FUNCIONAL.md
**Referencia UX:** N/A (backend only)

## 1. Visao geral do comportamento esperado

O plugin de moderacao de conteudo filtra input (prompt do usuario) e output (resposta do LLM) para bloquear ou sinalizar conteudo improprio. Atualmente utiliza wordlists e regex; planeja-se classificador ML (Llama Guard) para maior robustez. Categorias incluem hate, violence, sexual, self_harm, illegal, pii e custom.

**Limites conhecidos:**
- Wordlists e regex sao propensos a falsos positivos (ex.: "kill" em "kill the process").
- Wordlists em ingles nao cobrem portugues; multilinguagem exige wordlists por idioma.
- Modelo ML (Llama Guard) adiciona 100-500ms de latencia; uso opcional.
- Moderacao em streaming requer buffering de chunks.

### Comportamentos minimos

- Verificar texto do prompt (input) antes de enviar ao LLM.
- Verificar texto da resposta (output) antes de entregar ao cliente.
- Aplicar politicas por categoria (block, warn, log) conforme configuracao.
- Retornar HTTP 400 com mensagem padrao quando input bloqueado.
- Substituir ou sinalizar output bloqueado (resposta padrao ou header X-Content-Warning).
- Registrar violacoes quando log_violations=true.
- Expor estatisticas em endpoint admin.

## 2. Intents / comandos suportados

| Intent | Descricao |
|--------|-----------|
| **Input moderation** | Verificar prompt do usuario antes do envio ao LLM |
| **Output moderation** | Verificar resposta do LLM antes da entrega ao cliente |
| **Stats retrieval** | Obter estatisticas de moderacao via GET /admin/moderation/stats |

Acoes por categoria: block (bloquear), warn (permitir + log/warning), log (apenas registrar).

## 3. Contratos API (minimo)

**Request:** Nenhum endpoint novo. Intercepta `/v1/chat/completions`.

**Response em bloqueio (input):**
- HTTP 400
- Body: `{"error":{"message":"Content blocked: <category>","type":"content_policy_violation"}}`

**Header em warning (output):**
- `X-Content-Warning: <category> (<confidence>)`

**Endpoint admin:**
- `GET /admin/moderation/stats`
- Response: `{ total_checked, total_blocked, total_warned, by_category: { hate: {blocked, warned}, ... } }`

## 4. Fluxos detalhados

### Fluxo 1: Input bloqueado

1) Request chega em `/v1/chat/completions` com messages.
2) Plugin extrai texto do prompt (todas as mensagens user).
3) Aplica wordlists e regex por categoria.
4) Detecta match em categoria "illegal" com action=block.
5) Retorna HTTP 400 com mensagem "Content blocked: illegal".
6) Request nao e enviada ao LLM.
7) Incrementa total_blocked e by_category.illegal.blocked.

### Fluxo 2: Output com warning

1) LLM retorna resposta.
2) Plugin analisa texto da resposta.
3) Detecta match em categoria "illegal" com confidence 0.72 e action=warn.
4) Adiciona header X-Content-Warning: illegal (0.72).
5) Entrega resposta ao cliente (nao bloqueia).
6) Incrementa total_warned e by_category.illegal.warned.
7) Loga violacao se log_violations=true.

### Fluxo 3: Moderacao com modelo ML (futuro)

1) Wordlist/regex nao encontra match decisivo.
2) Opcionalmente chama modelo Llama Guard com texto.
3) Modelo retorna categorias e confidence.
4) Aplica threshold por categoria (ex.: 0.8 para block).
5) Se confidence >= threshold, aplica action configurada.

## 5. Validacoes e guardrails

- **Threshold por categoria:** Valores entre 0.0 e 1.0 para classificacao por modelo. Default 0.8. Calibrar para reduzir falsos positivos.
- **Categorias e acoes:** Cada categoria tem action (block/warn/log). Configuracao explicita evita over-moderation.
- **blocked_response:** Mensagem padrao retornada quando bloqueado; configurável.
- **moderate_output:** Flag para habilitar/desabilitar moderacao de output (default true).
- **Falsos positivos:** Wordlists devem ser curadas; considerar contexto (ex.: whitelist para termos tecnicos).

## 6. Seguranca e autorizacao

- Moderacao aplica-se a todas as requests; nao ha bypass por key (exceto se configurado explicitamente em extensao futura).
- Endpoint /admin/moderation/stats requer autorizacao admin.
- Logs de violacao nao devem expor conteudo completo em ambientes sensiveis; apenas categoria e metadata.

## 7. Telemetria e observabilidade

### Eventos recomendados

- `content_moderation_input_blocked` (category, matched_rule, key_alias)
- `content_moderation_output_blocked` (category, key_alias)
- `content_moderation_warned` (category, confidence, input|output)
- `content_moderation_ml_classification` (model, latency_ms, categories)

### Metadata minima

- traceId (X-Request-Id)
- model
- key_alias
- category
- confidence (quando aplicavel)
- action (block/warn/log)

## 8. Criterios de aceitacao (PoC/MVP)

- [ ] Input com palavra da wordlist "hate" bloqueado com HTTP 400.
- [ ] Input com regex de "how to make bomb" bloqueado.
- [ ] Output com conteudo sexual bloqueado e substituido por blocked_response.
- [ ] Output com confidence 0.72 em "illegal" gera X-Content-Warning e entrega resposta.
- [ ] GET /admin/moderation/stats retorna total_checked, total_blocked, total_warned, by_category.
- [ ] log_violations=true registra violacoes em log.

## Checklist de qualidade da especificacao AI

- [x] Intents mapeadas para fluxos reais
- [x] Guardrails e RBAC explicitos
- [x] Contratos API minimos definidos
- [x] Eventos e metadata de observabilidade definidos
- [x] Criterios de aceitacao objetivos e testaveis
