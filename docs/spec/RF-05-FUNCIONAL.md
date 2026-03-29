# RF-05 — Prompt Management

- **RF:** RF-05
- **Titulo:** Prompt Management
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Sistema centralizado de gerenciamento de prompts que permite injetar system prompts automaticamente por API key, criar templates reutilizaveis com variaveis, e aplicar guardrails basicos no conteudo dos prompts. Transforma o gateway de um proxy transparente em uma camada inteligente de controle de prompts.

## Escopo

- **Inclui:** PromptManager, PromptTemplate, GuardrailRule, system prompt injection por key, templates com variaveis, guardrails (block/warn/redact), endpoints admin de prompts
- **Nao inclui:** Versionamento de prompts com rollback, UI de edicao de templates

## Descricao Funcional Detalhada

### Arquitetura

Request do Cliente -> PromptManager -> (1) Inject system prompt -> (2) Apply template -> (3) Check guardrails -> (OK: Enviar ao Backend | Bloqueado: 400 Content Blocked) -> Response.

### Componentes

- **PromptManager:** Classe principal que coordena injection, templates e guardrails.
- **PromptTemplate:** Template com variaveis substituiveis (ex: {{content}}, {{language}}).
- **Guardrail:** Regra de validacao de conteudo (regex, action: block|warn|redact).

### Cenarios

1. **System prompt corporativo:** Todas as requests de uma key incluem "Voce e um assistente da empresa X. Responda sempre em portugues. Nunca divulgue informacoes confidenciais."
2. **Templates de prompt:** Template "summarize" que adiciona "Resuma o texto a seguir em 3 bullet points: {{content}}".
3. **Guardrails:** Bloquear prompts com padroes perigosos ou instrucoes de bypass.
4. **Versionamento:** (futuro) Manter versoes do system prompt e rollback.

## Interface / Contrato

```cpp
struct PromptTemplate {
    std::string name;
    std::string content;
    std::vector<std::string> variables;  // ex: ["content", "language"]
    int64_t created_at;
    int version;
};

struct GuardrailRule {
    std::string pattern;  // regex
    std::string action;   // "block" | "warn" | "redact"
    std::string message;  // mensagem de erro
};

class PromptManager {
public:
    void init(const std::string& config_path);
    void inject_system_prompt(Json::Value& body, const std::string& key_alias) const;
    [[nodiscard]] bool apply_template(Json::Value& body,
                                       const std::string& template_name,
                                       const Json::Value& variables) const;
    struct GuardrailResult {
        bool allowed;
        std::string reason;
        std::string rule_name;
    };
    [[nodiscard]] GuardrailResult check_guardrails(const Json::Value& messages) const;
    void set_key_system_prompt(const std::string& key_alias, const std::string& prompt);
    void add_template(const PromptTemplate& tmpl);
    void add_guardrail(const std::string& name, const GuardrailRule& rule);
    [[nodiscard]] Json::Value list_templates() const;
    [[nodiscard]] Json::Value list_guardrails() const;
};
```

## Configuracao

### config.json

```json
{
  "prompts": {
    "enabled": true,
    "default_system_prompt": "",
    "key_prompts": {
      "customer-support": "You are a helpful customer support agent for Company X. Always respond in Portuguese. Never share confidential information.",
      "code-assistant": "You are a senior software engineer. Write clean, well-documented code. Prefer modern C++23 patterns."
    },
    "templates": {
      "summarize": {
        "content": "Summarize the following text in {{num_points}} bullet points:\n\n{{content}}",
        "variables": ["num_points", "content"]
      },
      "translate": {
        "content": "Translate the following text to {{target_language}}:\n\n{{text}}",
        "variables": ["target_language", "text"]
      }
    },
    "guardrails": [
      {
        "name": "ignore_instructions",
        "pattern": "(?i)(ignore|forget|disregard).*(previous|above|prior).*(instructions?|prompts?|rules?)",
        "action": "block",
        "message": "Prompt injection detected"
      },
      {
        "name": "pii_warning",
        "pattern": "\\b\\d{3}\\.\\d{3}\\.\\d{3}-\\d{2}\\b",
        "action": "warn",
        "message": "PII detected in prompt (CPF)"
      }
    ]
  }
}
```

### Variaveis de Ambiente

| Variavel | Descricao | Default |
|----------|-----------|---------|
| `PROMPTS_ENABLED` | Habilitar gerenciamento de prompts | `false` |
| `DEFAULT_SYSTEM_PROMPT` | System prompt padrao para todas as keys | (vazio) |
| `GUARDRAILS_ENABLED` | Habilitar guardrails | `false` |

## Endpoints

| Metodo | Path | Descricao |
|--------|------|-----------|
| `GET` | `/admin/prompts/templates` | Listar templates |
| `POST` | `/admin/prompts/templates` | Criar template |
| `PUT` | `/admin/prompts/keys/{alias}` | Definir system prompt de uma key |
| `GET` | `/admin/prompts/guardrails` | Listar guardrails |

## Regras de Negocio

- Ordem de system prompts: definir politica quando cliente ja envia system prompt e key tem outro (prepend, append, replace).
- Template vars: usar header X-Prompt-Template ou campo extra no body (nao existe na API OpenAI padrao).
- Guardrails: compilar regex uma vez e cachear para performance.
- Template injection: variaveis nao podem permitir injection de instrucoes. Escapar {{}} no conteudo do usuario.
- Persistencia: templates e guardrails persistidos em disco para sobreviver restarts.

## Dependencias e Integracoes

- **Internas:** ApiKeyManager (associar prompt por key), Logger
- **Externas:** Nenhuma (regex via <regex> do C++23)

## Criterios de Aceitacao

- [ ] PromptManager injeta system prompt por key antes de enviar ao backend
- [ ] Templates com variaveis sao resolvidos corretamente
- [ ] Guardrails bloqueiam (block), avisam (warn) ou redactam (redact) conforme config
- [ ] PUT /admin/prompts/keys/{alias} define system prompt
- [ ] POST /admin/prompts/templates cria template
- [ ] GET /admin/prompts/templates e /admin/prompts/guardrails listam recursos
- [ ] Header X-Prompt-Template ou template_vars no body aplica template
- [ ] 400 Content Blocked quando guardrail bloqueia

## Riscos e Trade-offs

1. **Regex performance:** Guardrails com muitas regex podem impactar latencia. Compilar e cachear.
2. **Ordem de system prompts:** Politica (prepend, append, replace) precisa ser definida.
3. **Template injection:** Variaveis nao podem permitir injection. Escapar {{}}.
4. **Falsos positivos:** Guardrails de regex podem bloquear conteudo legitimo. Tunagem necessaria.
5. **Persistencia:** Templates e guardrails em disco.
6. **Compatibilidade OpenAI:** template_vars nao existe na API OpenAI. Usar headers customizados.

## Status de Implementacao

IMPLEMENTADO — PromptManagerPlugin implementado com: system prompt injection por key (prepend/append/replace), templates com variaveis ({{var}}), guardrails regex (block/warn/redact), persistencia em prompts.json, endpoints GET/POST /admin/prompts/templates, PUT /admin/prompts/keys/{alias}, GET /admin/prompts/guardrails, header X-Prompt-Template e X-Template-Vars. Action `redact` implementada nos guardrails (substitui conteudo correspondente por [REDACTED]). Guardrails persistidos em prompts.json junto com templates e key_prompts.

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
