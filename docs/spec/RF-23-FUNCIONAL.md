# RF-23 — Streaming Transformer

- **RF:** RF-23
- **Titulo:** Streaming Transformer
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Plugin que transforma chunks de streaming SSE em tempo real enquanto passam pelo gateway. Permite traducao on-the-fly, formatacao de markdown para HTML, censura de palavras em tempo real, e coleta de metricas de streaming (tokens/segundo, time-to-first-token).

## Escopo

- **Inclui:** Hook on_stream_chunk; transformacoes (regex_replace, append_suffix); metricas TTFT e tokens/s; censura em tempo real; traducao em tempo real (planejada).
- **Nao inclui:** Logica real de transformacao (before/after retornam Continue sem logica); interface StreamablePlugin estendida.

## Descricao Funcional Detalhada

### Fluxo

- Backend LLM envia chunks.
- Cada chunk passa pelo Transform Plugin.
- Chunk transformado e entregue ao cliente.
- Metricas: TTFT, chunk_count, total_tokens, tokens/s.

### Desafio Principal

Plugins normais tem before_request e after_response. Para streaming, e necessario hook adicional `on_stream_chunk` chamado para cada chunk SSE.

### Cenarios

1. Traducao em tempo real: modelo em ingles, chunk traduzido para portugues.
2. Formatacao: markdown para HTML on-the-fly.
3. Censura: substituir palavras proibidas em tempo real.
4. Metricas: TTFT e tokens/s para SLAs.
5. Transformacao de formato: Ollama para OpenAI em tempo real.

## Interface / Contrato

```cpp
class StreamablePlugin : public Plugin {
public:
    virtual bool supports_streaming() const { return false; }
    virtual std::optional<std::string> on_stream_chunk(const std::string& chunk, RequestContext& ctx) {
        return std::nullopt;  // pass-through
    }
    virtual void on_stream_start(RequestContext& ctx) {}
    virtual void on_stream_end(RequestContext& ctx) {}
};

class StreamingTransformerPlugin : public StreamablePlugin {
    void on_stream_start(RequestContext& ctx) override;
    std::optional<std::string> on_stream_chunk(const std::string& chunk, RequestContext& ctx) override;
    void on_stream_end(RequestContext& ctx) override;
};
```

## Configuracao

```json
{
  "plugins": {
    "pipeline": [
      {
        "name": "streaming_transformer",
        "enabled": true,
        "config": {
          "transforms": [
            { "type": "regex_replace", "pattern": "\\bbadword\\b", "replacement": "***" },
            { "type": "append_suffix", "suffix": "" }
          ],
          "collect_metrics": true,
          "log_ttft": true
        }
      }
    ]
  }
}
```

## Endpoints

N/A — plugin de pipeline com hook de streaming.

## Regras de Negocio

1. Transformacao <1ms por chunk para nao afetar experiencia.
2. Palavras cortadas: chunks podem cortar palavras; requer buffering de parciais.
3. Estado por stream em RequestContext.metadata.
4. Transformacoes lock-free para concorrencia.

## Dependencias e Integracoes

- **Internas:** Feature 10 (Plugin System) — estender interface com StreamablePlugin.
- **Externas:** Nenhuma.

## Criterios de Aceitacao

- [ ] Hook on_stream_chunk integrado no fluxo de streaming
- [ ] Transformacao regex_replace funcional
- [ ] Metricas TTFT e tokens/s coletadas
- [ ] Buffering de palavras parciais para censura correta
- [ ] before/after com logica real (nao apenas Continue)

## Riscos e Trade-offs

1. **Latencia por chunk:** Cada transformacao adiciona latencia; deve ser <1ms.
2. **Palavras cortadas:** "hel" + "lo world" dificulta replace de "hello"; buffering necessario.
3. **Traducao em tempo real:** Chunk por chunk produz traducoes ruins; bufferar frases completas.
4. **Interface estendida:** on_stream_chunk muda interface base; plugins existentes nao afetados (default pass-through).
5. **Concorrencia:** Multiplos streams com transformacao podem impactar throughput.

## Status de Implementacao

IMPLEMENTADO — StreamingTransformerPlugin implementado com: transform_chunk() integrado no streaming handler do gateway, regex_replace transforms, append_suffix, metricas TTFT e tokens/s, on_stream_start/on_stream_end lifecycle. Buffering de palavras parciais implementado para transforms de streaming; buffer retem palavras incompletas em limites de espaco em branco ate o proximo chunk; metodo flush_buffer() drena o buffer restante no fim do stream; configuravel via opcao `buffer_partial_words` (default: true).

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
