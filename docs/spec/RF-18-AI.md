# RF-18-AI — Request Router

- **RF:** RF-18
- **Titulo:** Request Router (auto)
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

**Referencia funcional:** RF-18-FUNCIONAL.md
**Referencia UX:** N/A (backend only)

## 1. Visao geral do comportamento esperado

O plugin analisa o conteudo do prompt quando o cliente envia `model: "auto"` (ou trigger_models configurados) e roteia para o modelo mais adequado. A classificacao e feita por keywords/regex e analise de complexidade (tokens). Planeja-se classificador LLM para maior precisao. Tipos de tarefa: code, translation, summarization, analysis, creative, math, general, simple.

**Limites conhecidos:**
- Classificacao por keywords e imprecisa (ex.: "Can you code me a poem?" seria code erroneamente).
- Classificacao por LLM adiciona latencia (100-500ms).
- Modelo selecionado pode estar indisponivel; fallback nao especificado.
- Cache por modelo fica fragmentado com model=auto.

### Comportamentos minimos

- Interceptar requests com model em trigger_models (ex.: "auto", "default").
- Classificar tipo de tarefa pelo conteudo (keywords, regex, complexity).
- Selecionar modelo conforme rules (task -> model) e priority.
- Substituir model no body antes de encaminhar ao backend.
- Adicionar headers X-Routed-Model e X-Task-Type na resposta.
- Usar default_model quando classificacao nao encontra match ou task=general.

## 2. Intents / comandos suportados

| Intent | Descricao |
|--------|-----------|
| **Auto-routing** | Classificar e rotear quando model=auto |
| **Task classification** | Identificar tipo: code, translation, summarization, math, simple, complex, general |
| **Model selection** | Mapear task para modelo via rules |
| **Fallback** | Usar default_model quando classificacao inconclusiva |

Nao ha comandos explicitos; o cliente envia model: "auto" e o gateway decide.

## 3. Contratos API (minimo)

**Request:** Nenhum endpoint novo. Intercepta `/v1/chat/completions` quando model em trigger_models.

**Headers de resposta:**
- `X-Routed-Model: <modelo_selecionado>`
- `X-Task-Type: <task_type>`

**Configuracao (keyword_classification):**
- Mapeamento task -> lista de keywords para match no texto.
- complexity_threshold: simple_max_tokens, complex_min_tokens para classificar simple/complex.

**Request body:** O campo `model` e substituido internamente pelo modelo selecionado antes do envio ao backend.

## 4. Fluxos detalhados

### Fluxo 1: Request de codigo

1) Request chega com model: "auto" e content: "Write a Python function to sort a list".
2) Plugin verifica model em trigger_models; match.
3) Extrai texto das messages.
4) Classifica por keywords: "function", "Python" -> task=code.
5) Rules: task code -> deepseek-coder:6.7b.
6) Substitui model no body por "deepseek-coder:6.7b".
7) Encaminha ao backend.
8) Na resposta, adiciona X-Routed-Model: deepseek-coder:6.7b, X-Task-Type: code.

### Fluxo 2: Request simples

1) Request com content: "What is 2+2?" (poucos tokens).
2) Classificacao por complexity: tokens < simple_max_tokens (50) -> task=simple.
3) Rules: task simple -> gemma3:1b.
4) Roteia para gemma3:1b.
5) Headers: X-Routed-Model: gemma3:1b, X-Task-Type: simple.

### Fluxo 3: Request de traducao

1) Request com content: "Translate to English: Bom dia, como voce esta?"
2) Keywords: "translate", "Translate" -> task=translation.
3) Rules: task translation -> gemma3:4b.
4) Roteia para gemma3:4b.
5) Headers: X-Routed-Model: gemma3:4b, X-Task-Type: translation.

### Fluxo 4: Conteudo desconhecido / fallback

1) Request com content generico sem keywords conhecidas.
2) Classificacao retorna task=general ou inconclusiva.
3) Usa default_model (ex.: llama3:8b).
4) Roteia para default_model.
5) Headers: X-Routed-Model: llama3:8b, X-Task-Type: general.

## 5. Validacoes e guardrails

- **Validacao de rota:** Modelo selecionado deve existir no provider; se indisponivel, fallback para default_model (comportamento a definir).
- **Conteudo desconhecido:** Sempre retornar default_model; nunca falhar a request.
- **Transparencia:** Cliente deve receber headers indicando modelo e task usados para reproducibilidade.
- **Ordem de rules:** Priority (menor = maior prioridade) para resolver conflitos quando multiplas rules aplicam.
- **Modelo nao em trigger_models:** Request passa sem alteracao (model original preservado).

## 6. Seguranca e autorizacao

- Router nao altera RBAC. A key do cliente deve ter acesso ao modelo selecionado.
- Se key nao tem permissao para o modelo roteado, o backend rejeita; router nao valida permissao por modelo.
- Config por key (ex.: customer-support sempre usa modelo X) pode ser extensao futura.

## 7. Telemetria e observabilidade

### Eventos recomendados

- `request_router_classification` (task, confidence, model_selected, key_alias)
- `request_router_fallback` (reason: unknown_content|model_unavailable, default_model)
- `request_router_llm_classification` (model, latency_ms, task) — quando use_classifier_model=true

### Metadata minima

- traceId (X-Request-Id)
- model (original, ex.: "auto")
- routed_model (modelo selecionado)
- task_type
- key_alias
- confidence (quando classificador LLM)

## 8. Criterios de aceitacao (PoC/MVP)

- [ ] Request com model: "auto" e "Write a Python function" roteada para deepseek-coder.
- [ ] Request com "What is 2+2?" roteada para modelo simple (gemma3:1b).
- [ ] Request com "Translate to English: ..." roteada para gemma3:4b.
- [ ] Headers X-Routed-Model e X-Task-Type presentes em todas as respostas roteadas.
- [ ] Request com conteudo generico usa default_model.
- [ ] Request com model diferente de trigger_models nao e alterada.

## Checklist de qualidade da especificacao AI

- [x] Intents mapeadas para fluxos reais
- [x] Guardrails e RBAC explicitos
- [x] Contratos API minimos definidos
- [x] Eventos e metadata de observabilidade definidos
- [x] Criterios de aceitacao objetivos e testaveis
