# RF-01 — Multi-Provider

- **RF:** RF-01
- **Titulo:** Multi-Provider
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Expandir o HERMES para suportar multiplos providers de LLM alem do Ollama. O gateway roteia requests para Ollama, OpenRouter (fallback padrao quando Ollama falhar), OpenAI, Anthropic (Claude), Groq, Mistral, Together AI e providers custom (qualquer API OpenAI-compativel), mantendo uma interface unificada para o cliente.

O cliente continua usando a mesma API OpenAI-compativel. O gateway decide para qual provider enviar com base no modelo solicitado. Por padrao apenas Ollama fica habilitado; demais providers sao desabilitados ate configuracao explicita via `config.json` ou variaveis de ambiente.

## Escopo

- **Inclui:** ProviderRouter, OllamaProvider, OpenAICustomProvider, OpenRouterProvider, OpenAIProvider, AnthropicProvider, GroqProvider, model routing, fallback chain, configuracao via config.json e env vars
- **Nao inclui:** Rate limits externos dos providers, billing por provider, UI de configuracao

## Descricao Funcional Detalhada

### Arquitetura

O fluxo e: Cliente HTTP -> HERMES -> ProviderRouter -> Provider especifico (Ollama, OpenRouter, OpenAI, Anthropic, Custom) -> Backend do provider.

O ProviderRouter decide qual provider usar com base no modelo solicitado. Quando o provider primario falha (timeout, 502), aplica a fallback chain (ex: ollama -> openrouter).

### Componentes

- **Provider (interface base):** Define o contrato que todo provider deve implementar.
- **OllamaProvider:** Refatoracao do OllamaClient atual para implementar a interface.
- **OpenRouterProvider:** Proxy direto para OpenRouter (API OpenAI-compativel); fallback padrao quando Ollama falhar.
- **OpenAIProvider:** Proxy direto — a request ja esta no formato OpenAI.
- **AnthropicProvider:** Transforma request OpenAI -> formato Anthropic Messages API e vice-versa.
- **GroqProvider:** Proxy direto (API compativel com OpenAI).
- **CustomProvider:** Provider generico para qualquer API OpenAI-compativel (Azure, proxies locais, APIs proprietarias); formato padrao OpenAI.
- **ProviderRouter:** Decide qual provider usar com base no modelo solicitado; aplica fallback chain quando provider primario falha.

### Cenarios

1. **Hibrido local/cloud:** Usar Ollama para requests simples e OpenAI GPT-4o para requests complexas.
2. **Fallback:** Se o Ollama local cair, redirecionar automaticamente para OpenRouter ou outros providers na fallback_chain.
3. **Multi-tenant:** Diferentes API keys podem ter acesso a diferentes providers.
4. **Migracao gradual:** Trocar de provider sem alterar nenhum cliente.

## Interface / Contrato

```cpp
struct ProviderResponse {
    int status_code;
    std::string body;
    bool is_stream;
};

struct RequestContext {
    std::string request_id;
    std::string key_alias;
    std::string client_ip;
};

class Provider {
public:
    virtual ~Provider() = default;
    [[nodiscard]] virtual std::string name() const = 0;
    [[nodiscard]] virtual bool supports_model(const std::string& model) const = 0;
    [[nodiscard]] virtual httplib::Result chat_completion(
        const std::string& body, const RequestContext& ctx) = 0;
    [[nodiscard]] virtual httplib::Result embeddings(
        const std::string& body, const RequestContext& ctx) = 0;
    [[nodiscard]] virtual httplib::Result list_models(const RequestContext& ctx) = 0;
    virtual void chat_completion_stream(...) = 0;
    [[nodiscard]] virtual bool is_healthy() const = 0;
};

class ProviderRouter {
public:
    void register_provider(std::unique_ptr<Provider> provider);
    void set_model_mapping(const std::string& model, const std::string& provider_name);
    void set_fallback_chain(const std::vector<std::string>& provider_names);
    [[nodiscard]] Provider* resolve(const std::string& model) const;
    [[nodiscard]] Provider* fallback_for(const std::string& model) const;
};
```

## Configuracao

### config.json

```json
{
  "providers": {
    "ollama": { "enabled": true, "default_model": "llama3:8b", "backends": ["http://localhost:11434"] },
    "openrouter": { "enabled": false, "default_model": "openai/gpt-4o-mini", "base_url": "https://openrouter.ai/api/v1", "api_key_env": "OPENROUTER_API_KEY" },
    "openai": { "enabled": false, "default_model": "gpt-4o", "api_key_env": "OPENAI_API_KEY", "base_url": "https://api.openai.com" },
    "anthropic": { "enabled": false, "default_model": "claude-3.5-sonnet", "api_key_env": "ANTHROPIC_API_KEY", "base_url": "https://api.anthropic.com" },
    "groq": { "enabled": false, "default_model": "mixtral-8x7b", "api_key_env": "GROQ_API_KEY", "base_url": "https://api.groq.com/openai" },
    "custom": { "enabled": false, "id": "custom-openai", "base_url": "https://api.example.com/v1", "api_key_env": "CUSTOM_OPENAI_API_KEY", "default_model": "gpt-4" }
  },
  "model_routing": {
    "gpt-4o": "openai",
    "gpt-3.5-turbo": "openai",
    "claude-3.5-sonnet": "anthropic",
    "llama3:8b": "ollama",
    "mixtral-8x7b": "groq"
  },
  "fallback_chain": ["ollama", "openrouter"]
}
```

### Variaveis de Ambiente

| Variavel | Descricao | Default |
|----------|-----------|---------|
| `OLLAMA_ENABLED` | Habilitar provider Ollama | `true` |
| `OLLAMA_DEFAULT_MODEL` | Modelo padrao do Ollama | `llama3:8b` |
| `OPENROUTER_ENABLED` | Habilitar provider OpenRouter | `false` |
| `OPENROUTER_API_KEY` | API key OpenRouter | (vazio) |
| `OPENAI_ENABLED` | Habilitar provider OpenAI | `false` |
| `OPENAI_API_KEY` | Chave da API OpenAI | (vazio) |
| `ANTHROPIC_ENABLED` | Habilitar provider Anthropic | `false` |
| `ANTHROPIC_API_KEY` | Chave da API Anthropic | (vazio) |
| `GROQ_ENABLED` | Habilitar provider Groq | `false` |
| `GROQ_API_KEY` | Chave da API Groq | (vazio) |
| `CUSTOM_PROVIDER_ENABLED` | Habilitar provider custom | `false` |
| `CUSTOM_PROVIDER_BASE_URL` | Base URL do custom | (vazio) |
| `CUSTOM_PROVIDER_API_KEY` | API key do custom | (vazio) |
| `FALLBACK_CHAIN` | Chain de fallback (ex: `ollama,openrouter,openai`) | `ollama,openrouter` |

## Endpoints

| Metodo | Path | Descricao |
|--------|------|-----------|
| (existente) | `/v1/chat/completions` | Roteado internamente para o provider correto |
| (existente) | `/v1/models` | Roteado internamente |
| (futuro) | `GET /admin/providers` | Lista providers configurados e status de saude |

## Regras de Negocio

- Provider desabilitado: nao e registrado no ProviderRouter, nao aparece em model_routing, nao participa do fallback.
- Modelo padrao: usado quando o modelo solicitado nao esta em model_routing e o provider e escolhido por outro criterio (ex: fallback).
- Fallback chain: default `["ollama", "openrouter"]` — quando Ollama falhar (timeout, 502), tentar OpenRouter. So usa providers habilitados.
- Provider custom: usa formato OpenAI (proxy direto); ideal para Azure OpenAI, proxies locais, APIs proprietarias OpenAI-compativel.
- API keys dos providers: armazenadas via variaveis de ambiente, nunca no config.json em disco.

## Dependencias e Integracoes

- **Internas:** httplib (ja presente) para requests HTTPS aos providers cloud
- **Externas:** Nenhuma nova. httplib suporta HTTPS com OpenSSL (-lssl -lcrypto)
- **Pre-requisito:** Compilar httplib com CPPHTTPLIB_OPENSSL_SUPPORT para HTTPS

## Criterios de Aceitacao

- [ ] ProviderRouter roteia requests para o provider correto baseado no modelo
- [ ] Fallback chain funciona quando provider primario falha (timeout, 502)
- [ ] Ollama, OpenRouter, OpenAI, Anthropic, Groq e Custom providers implementam interface Provider
- [ ] Configuracao via config.json e env vars (env sobrescreve JSON)
- [ ] Cliente usa mesma API OpenAI-compativel sem alteracoes
- [ ] Provider desabilitado nao participa do roteamento nem fallback

## Riscos e Trade-offs

1. **HTTPS:** httplib precisa ser compilado com suporte SSL. Adicionar OpenSSL como dependencia no Docker build.
2. **Transformacao Anthropic:** API Anthropic tem formato diferente (Messages API). Transformacao bidirecional complexa, especialmente para streaming.
3. **Rate limits externos:** Providers cloud tem rate limits proprios. Gateway precisa lidar com 429s sem confundir com rate limits proprios.
4. **Custos:** Requests para providers cloud tem custo. Sem controle, pode gerar gastos inesperados.
5. **Latencia:** Providers cloud adicionam latencia de rede. Fallback precisa ser rapido.
6. **OpenRouter:** Depende de OPENROUTER_API_KEY; sem ela, fallback para OpenRouter falhara.

## Status de Implementacao

IMPLEMENTADO — ProviderRouter, OllamaProvider, OpenAICustomProvider e fallback chain implementados. Suporte a multiplos providers via config.json e env vars.

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
