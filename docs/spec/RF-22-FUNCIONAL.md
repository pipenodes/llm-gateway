# RF-22 — Logging Exporter

- **RF:** RF-22
- **Titulo:** Logging Exporter
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Plugin que exporta logs, metricas e traces para plataformas externas de observabilidade. Suporta envio para Datadog, Grafana Cloud, Elasticsearch/Loki, e qualquer endpoint compativel com OpenTelemetry (OTLP). Permite monitoramento e alertas sem depender dos endpoints de metricas do gateway.

## Escopo

- **Inclui:** Logging estruturado JSON (LoggingPlugin); batch collector; exportadores planejados (OTLP, Datadog, Loki, Elasticsearch); traces distribuidos; logs estruturados.
- **Nao inclui:** Exportadores OTLP/Datadog/Loki/Elasticsearch ainda nao implementados (planejados).

## Descricao Funcional Detalhada

### Arquitetura

- Request/Response passa pelo plugin.
- Batch Collector acumula dados.
- Flush periodico envia para Exporters.
- Destinos planejados: OTLP, Datadog API, Grafana Loki, Elasticsearch, Custom HTTP.

### Cenarios

1. Grafana + Prometheus: metricas detalhadas alem do /metrics/prometheus.
2. Datadog APM: traces distribuidos (gateway + backend + streaming).
3. Elasticsearch: logs estruturados indexados para busca e Kibana.
4. Alertas: configurar alertas em Datadog/Grafana baseados em metricas.

## Interface / Contrato

```cpp
struct TraceSpan {
    std::string trace_id, span_id, parent_span_id;
    std::string operation;
    int64_t start_us, duration_us;
    std::unordered_map<std::string, std::string> tags;
};

struct LogEntry {
    int64_t timestamp;
    std::string level, message;
    Json::Value fields;
};

struct MetricPoint {
    std::string name;
    double value;
    int64_t timestamp;
    std::unordered_map<std::string, std::string> labels;
};

class ExporterBackend {
    virtual bool send_traces(const std::vector<TraceSpan>& spans) = 0;
    virtual bool send_logs(const std::vector<LogEntry>& logs) = 0;
    virtual bool send_metrics(const std::vector<MetricPoint>& metrics) = 0;
};

class LoggingExporterPlugin : public Plugin { ... };
```

## Configuracao

```json
{
  "plugins": {
    "pipeline": [
      {
        "name": "logging_exporter",
        "enabled": true,
        "config": {
          "flush_interval_ms": 5000,
          "batch_size": 100,
          "exporters": [
            { "type": "otlp", "endpoint": "http://otel-collector:4318/v1/traces", "service_name": "hermes" },
            { "type": "loki", "endpoint": "http://loki:3100/loki/api/v1/push", "labels": {"app": "hermes"} },
            { "type": "elasticsearch", "endpoint": "http://elasticsearch:9200", "index": "hermes-logs" }
          ]
        }
      }
    ]
  }
}
```

### Variaveis de Ambiente

| Variavel | Descricao | Default |
|---|---|---|
| `OTEL_EXPORTER_OTLP_ENDPOINT` | Endpoint OTLP | (vazio) |
| `OTEL_SERVICE_NAME` | Nome do servico | `hermes` |
| `LOKI_ENDPOINT` | Endpoint Grafana Loki | (vazio) |
| `DATADOG_API_KEY` | API key Datadog | (vazio) |
| `ELASTICSEARCH_URL` | URL Elasticsearch | (vazio) |

## Endpoints

N/A — plugin de pipeline; exporta via HTTP para destinos configurados.

## Regras de Negocio

1. Logging estruturado JSON em stdout (implementado).
2. Batch flush a cada flush_interval_ms (default 5000).
3. Credenciais nao logadas nem expostas.
4. Sampling opcional para alto volume (ex: 10% dos traces).

## Dependencias e Integracoes

- **Internas:** Feature 10 (Plugin System), httplib (envio).
- **Externas:** Plataformas destino (Datadog, Grafana, Elasticsearch).

## Criterios de Aceitacao

- [ ] Logging estruturado JSON funcional (LoggingPlugin)
- [ ] Batch collector com flush periodico
- [ ] Suporte a OTLP (planejado)
- [ ] Suporte a Loki (planejado)
- [ ] Suporte a Elasticsearch (planejado)
- [ ] Suporte a Datadog (planejado)

## Riscos e Trade-offs

1. **Overhead:** Traces detalhados adicionam overhead; usar sampling.
2. **HTTPS:** Exporters cloud requerem HTTPS.
3. **Falha do destino:** Dados perdidos apos buffer encher; considerar dead letter file.
4. **Volume:** Filtrar e amostrar em alto throughput.
5. **Credenciais:** API keys sao secrets; nao logar nem expor.

## Status de Implementacao

IMPLEMENTADO — LoggingPlugin com logging estruturado JSON. Exportadores OTLP, Datadog, Loki, Elasticsearch planejados.

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
