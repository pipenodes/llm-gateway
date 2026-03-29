# RF-24 — API Versioning

- **RF:** RF-24
- **Titulo:** API Versioning
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Plugin que suporta multiplas versoes da API OpenAI simultaneamente. Detecta a versao via header ou path, transforma requests e responses entre versoes, e adiciona deprecation warnings para versoes antigas. Permite migracao gradual dos clientes sem breaking changes.

## Escopo

- **Inclui:** Deteccao de versao (OpenAI-Version header); transformacao request/response entre versoes; deprecation warnings; compatibilidade retroativa; suporte a versoes 2023-06-01, 2024-01-01, 2024-06-01.
- **Nao inclui:** Versionamento de API propria do gateway; transformacoes de streaming (tool_calls em chunks).

## Descricao Funcional Detalhada

### Fluxo

1. Request chega; detecta versao (v1 antiga ou v2 atual).
2. Se v1: transforma V1 -> V2.
3. Se v2: pass-through.
4. Core processa em V2.
5. Response V2; se cliente V1: transforma V2 -> V1 + Deprecation header.
6. Se cliente V2: entrega direto.

### Transformacoes V1 (2023-06-01) -> V2 (2024-06-01)

| Campo V1 | Campo V2 | Descricao |
|---|---|---|
| `functions` | `tools` | Array de funcoes |
| `function_call` | `tool_choice` | Selecao de funcao |
| `message.function_call` | `message.tool_calls` | Chamada na response |

## Interface / Contrato

```cpp
struct VersionTransform {
    std::string from_version, to_version;
    std::function<void(Json::Value&)> transform_request;
    std::function<void(Json::Value&)> transform_response;
};

class APIVersioningPlugin : public Plugin {
    PluginResult before_request(Json::Value& body, RequestContext& ctx) override;
    PluginResult after_response(Json::Value& response, RequestContext& ctx) override;
private:
    std::string detect_version(const RequestContext& ctx) const;
    void apply_deprecation_headers(Json::Value& response, const std::string& client_version) const;
};
```

## Configuracao

```json
{
  "plugins": {
    "pipeline": [
      {
        "name": "api_versioning",
        "enabled": true,
        "config": {
          "current_version": "2024-06-01",
          "supported_versions": ["2023-06-01", "2024-01-01", "2024-06-01"],
          "deprecated_versions": {
            "2023-06-01": {"eol_date": "2025-06-01", "message": "Please upgrade to 2024-06-01"}
          },
          "version_header": "OpenAI-Version",
          "default_version": "2024-06-01"
        }
      }
    ]
  }
}
```

## Endpoints

N/A — plugin de pipeline; aplica-se a todos os endpoints de chat/completions.

### Exemplo de Uso

```bash
# Request versao antiga
curl -X POST http://localhost:8080/v1/chat/completions \
  -H "OpenAI-Version: 2023-06-01" \
  -d '{"model":"llama3:8b","messages":[...],"functions":[...]}'
# Plugin transforma functions -> tools; response tool_calls -> function_call
# Headers: Deprecation: true, Sunset: 2025-06-01

# Request versao atual
curl -X POST http://localhost:8080/v1/chat/completions \
  -H "OpenAI-Version: 2024-06-01" \
  -d '{"model":"llama3:8b","messages":[...],"tools":[...]}'
# Pass-through
```

## Regras de Negocio

1. Header OpenAI-Version determina versao do cliente.
2. Versoes deprecated recebem Deprecation: true e Sunset com data EOL.
3. default_version quando header ausente.
4. Transformacoes JSON adicionam overhead minimo.

## Dependencias e Integracoes

- **Internas:** Feature 10 (Plugin System).
- **Externas:** Nenhuma.

## Criterios de Aceitacao

- [ ] Deteccao de versao via header OpenAI-Version
- [ ] Transformacao functions -> tools (request)
- [ ] Transformacao tool_calls -> function_call (response)
- [ ] Deprecation headers em versoes antigas
- [ ] Pass-through para versao atual

## Riscos e Trade-offs

1. **Complexidade:** Cada par de versoes tem muitas diferencas; manter transformacoes e trabalhoso.
2. **Testes:** Cada versao precisa de testes especificos.
3. **Streaming:** Transformacoes em streaming mais complexas.
4. **Documentacao:** Cada versao precisa de documentacao ou notas de migracao.

## Status de Implementacao

IMPLEMENTADO — OpenAI-Version header, version metadata, deprecation warnings.

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
