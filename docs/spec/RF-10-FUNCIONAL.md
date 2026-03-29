# RF-10 — Plugin/Middleware System

- **RF:** RF-10
- **Titulo:** Plugin/Middleware System
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Sistema extensivel de plugins que permite adicionar funcionalidades ao gateway sem modificar o codigo core. Plugins implementam uma interface padrao com hooks before_request e after_response, sendo executados em cadeia (pipeline). Plugins podem ser builtin (compilados junto) ou carregados dinamicamente via shared libraries (.so). Suporta service plugins: ICache, IAuth, IRateLimiter, IRequestQueue.

## Escopo

- **Inclui:** Interface Plugin, PluginManager, pipeline before/after, RequestContext, PluginResult (Continue/Block/Skip), builtin e dynamic (.so), configuracao pipeline, endpoint /admin/plugins
- **Nao inclui:** Hook on_stream_chunk para streaming, ABI stability garantida para plugins dinamicos

## Descricao Funcional Detalhada

### Arquitetura

Request HTTP -> Plugin Pipeline (before_request: P1 -> P2 -> P3 -> Core) -> Gateway Core -> Plugin Pipeline (after_response: P3 -> P2 -> P1) -> Response HTTP.

### Componentes

- **Plugin:** Interface base com name(), version(), init(), before_request(), after_response(), shutdown().
- **PluginManager:** Gerencia ciclo de vida, load_builtin(), load_dynamic(path), run_before_request(), run_after_response().
- **PluginResult:** Continue (proximo plugin), Block (retornar erro), Skip (pular restantes e enviar ao backend).
- **RequestContext:** request_id, key_alias, client_ip, model, method, path, is_stream, metadata (chave-valor).

### Service Plugins

Interfaces opcionais para substituir implementacoes core: ICache, IAuth, IRateLimiter, IRequestQueue. Plugins podem implementar essas interfaces para prover servicos customizados.

## Interface / Contrato

```cpp
struct RequestContext {
    std::string request_id;
    std::string key_alias;
    std::string client_ip;
    std::string model;
    std::string method;
    std::string path;
    bool is_stream;
    std::unordered_map<std::string, std::string> metadata;
};

enum class PluginAction { Continue, Block, Skip };

struct PluginResult {
    PluginAction action = PluginAction::Continue;
    int error_code = 0;
    std::string error_message;
};

class Plugin {
public:
    virtual ~Plugin() = default;
    [[nodiscard]] virtual std::string name() const = 0;
    [[nodiscard]] virtual std::string version() const = 0;
    virtual bool init(const Json::Value& config) = 0;
    virtual PluginResult before_request(Json::Value& body, RequestContext& ctx) = 0;
    virtual PluginResult after_response(Json::Value& response, RequestContext& ctx) = 0;
    virtual void shutdown() {}
};

class PluginManager {
public:
    void init(const Json::Value& config);
    template<typename T> void register_builtin();
    bool load_dynamic(const std::string& path);
    PluginResult run_before_request(Json::Value& body, RequestContext& ctx);
    PluginResult run_after_response(Json::Value& response, RequestContext& ctx);
    [[nodiscard]] Json::Value list_plugins() const;
    void shutdown_all();
};

// Plugins dinamicos (.so)
extern "C" {
    using PluginCreateFn = Plugin* (*)();
    using PluginDestroyFn = void (*)(Plugin*);
}
```

## Configuracao

### config.json

```json
{
  "plugins": {
    "enabled": true,
    "pipeline": [
      {
        "name": "pii_redactor",
        "enabled": true,
        "config": { "patterns": ["cpf", "email", "phone"] }
      },
      {
        "name": "content_moderator",
        "enabled": true,
        "config": { "block_categories": ["hate", "violence"] }
      }
    ],
    "dynamic_plugins_dir": "plugins/"
  }
}
```

### Variaveis de Ambiente

| Variavel | Descricao | Default |
|----------|-----------|---------|
| `PLUGINS_ENABLED` | Habilitar sistema de plugins | `false` |
| `PLUGINS_DIR` | Diretorio de plugins dinamicos | `plugins/` |

## Endpoints

| Metodo | Path | Descricao |
|--------|------|-----------|
| `GET` | `/admin/plugins` | Listar plugins carregados e status |

### Response

```json
{
  "plugins": [
    {
      "name": "pii_redactor",
      "version": "1.0.0",
      "type": "builtin",
      "enabled": true,
      "position": 0
    }
  ]
}
```

## Regras de Negocio

- Ordem dos plugins no pipeline afeta o resultado; documentar ordem esperada.
- Plugin com Block interrompe pipeline e retorna erro ao cliente.
- Plugin com Skip pula plugins restantes e envia direto ao backend.
- Plugins dinamicos (.so) requerem mesmo compilador e flags; ABI instavel.
- try/catch ao redor de cada plugin call para evitar crash do gateway.

## Dependencias e Integracoes

- **Internas:** gateway.cpp (integracao no pipeline)
- **Externas:** libdl (Linux) para dlopen/dlsym
- **C++23:** std::unique_ptr, templates

## Criterios de Aceitacao

- [ ] PluginManager executa pipeline before_request e after_response
- [ ] Plugins builtin registravel via register_builtin
- [ ] Plugins dinamicos carregaveis via load_dynamic
- [ ] PluginResult Block/Skip/Continue respeitados
- [ ] RequestContext compartilhado entre plugins
- [ ] Endpoint /admin/plugins lista plugins e status

## Riscos e Trade-offs

1. **Performance:** Cada plugin adiciona overhead; manter interface leve.
2. **Seguranca:** Plugins .so tem acesso total ao processo; so carregar confiados.
3. **Ordem importa:** Documentar ordem esperada do pipeline.
4. **Streaming:** after_response nao funciona trivialmente com chunks SSE; hook on_stream_chunk futuro.
5. **Erros:** Plugin com bug pode crashar; try/catch obrigatorio.
6. **ABI stability:** Mudancas na interface Plugin quebram plugins dinamicos.
7. **Debug:** Logging detalhado do pipeline para diagnostico.

## Status de Implementacao

IMPLEMENTADO — Sistema de plugins com interface Plugin, PluginManager, pipeline before/after, builtin e dynamic (.so), service plugins (ICache, IAuth, IRateLimiter, IRequestQueue).

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
