# RF-12 — Dynamic Discovery

- **RF:** RF-12
- **Titulo:** Dynamic Discovery
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Implementar descoberta dinamica de backends/providers com hot reload, inspirado no modelo do Traefik. O gateway descobre backends automaticamente a partir de tres fontes de configuracao (Docker, Kubernetes, File), sem banco de dados, sem coordenacao entre instancias, e sem downtime na reconfiguracao.

## Escopo

- **Inclui:** Docker provider (labels), Kubernetes provider (annotations), File provider (YAML/JSON), DynamicConfigFragment, ConfigurationWatcher, merge de fragmentos, validacao pre-swap, debounce/throttling, hot reload
- **Nao inclui:** Banco de dados, coordenacao entre instancias do gateway

## Descricao Funcional Detalhada

### Modelo Arquitetural

Providers (Docker, K8s, File) -> DynamicConfigFragment (parcial por provider) -> ConfigurationWatcher -> merge fragments (priority order) -> validate (pre-swap) -> debounce (500ms + min 1s reload) -> ProviderRouter (immutable, shared_ptr) -> atomic swap -> Gateway request handling (zero locks).

### Principios

- Config separada em estatica (startup) e dinamica (runtime hot reload).
- Providers retornam fragmentos parciais (nao config completa).
- Cada backend tem namespace por provider (name@provider) para evitar colisao.
- Validacao da config merged antes de aplicar; se invalida, mantem router anterior.
- Stateless: multiplas instancias descobrem os mesmos backends sem coordenacao.

### Fontes de Configuracao

1. **Docker Provider:** Labels hermes.* via Docker API (Unix socket ou TCP). Polling a cada N segundos; hash do snapshot para detectar mudancas.
2. **Kubernetes Provider:** Annotations hermes.io/* via K8s REST API. Polling com hash; suporta in-cluster (service account) e kubeconfig externo.
3. **File Provider:** YAML/JSON em diretorio configurado. Vigia via stat() (mtime + size).

## Interface / Contrato

- **IConfigProvider:** Interface que cada provider implementa para retornar DynamicConfigFragment.
- **DynamicConfigFragment:** Backends, model_routing, model_aliases, fallback_chain parciais.
- **ConfigurationWatcher:** Agregador central; merge, debounce, validacao, swap atomico.
- **ConfigValidator:** Valida model_routing, fallback_chain, model_aliases, backends antes de aplicar.

### Docker Labels

| Label | Descricao |
|-------|-----------|
| hermes.enable | true |
| hermes.provider.name | Nome do backend |
| hermes.provider.type | ollama ou openai |
| hermes.provider.port | Porta |
| hermes.provider.models | Modelos (comma-sep) |
| hermes.provider.default-model | Modelo padrao |
| hermes.provider.weight | Peso load balancing |
| hermes.model-routing.<model> | Roteamento |

### Kubernetes Annotations

| Annotation | Descricao |
|------------|-----------|
| hermes.io/enable | true |
| hermes.io/provider-name | Nome |
| hermes.io/provider-type | ollama ou openai |
| hermes.io/port | Porta |
| hermes.io/models | Modelos |
| hermes.io/default-model | Modelo padrao |
| hermes.io/weight | Peso |
| hermes.io/model-routing.<model> | Roteamento |

## Configuracao

### config.json (secao discovery)

```json
{
  "discovery": {
    "docker": {
      "enabled": false,
      "endpoint": "unix:///var/run/docker.sock",
      "poll_interval_seconds": 5,
      "label_prefix": "hermes",
      "network": ""
    },
    "kubernetes": {
      "enabled": false,
      "api_server": "",
      "namespace": "",
      "label_selector": "hermes.io/enable=true",
      "poll_interval_seconds": 10,
      "use_watch": true
    },
    "file": {
      "enabled": false,
      "paths": ["/etc/hermes/dynamic/"],
      "watch": true,
      "poll_interval_seconds": 5
    },
    "merge_priority": ["file", "kubernetes", "docker"]
  }
}
```

### Variaveis de Ambiente

DISCOVERY_DOCKER_ENABLED, DISCOVERY_DOCKER_ENDPOINT, DISCOVERY_K8S_ENABLED, DISCOVERY_K8S_NAMESPACE, DISCOVERY_FILE_ENABLED, DISCOVERY_FILE_PATHS, etc.

## Endpoints

| Metodo | Path | Descricao |
|--------|------|-----------|
| `GET` | `/admin/discovery` | Status de cada provider (healthy, backends, latencia) |
| `POST` | `/admin/discovery/refresh` | Forca re-scan imediato |
| `GET` | `/admin/config` | Config final merged (o que o gateway usa agora) |

## Regras de Negocio

- **Merge:** Prioridade configuravel (default: file > kubernetes > docker). Backends: union. Model routing/aliases/fallback: provider de maior prioridade sobrescreve.
- **Validacao pre-swap:** model_routing aponta para backend existente; fallback_chain contem backends existentes; model_aliases sem ciclos; backends com base_url e type validos; qualified_id unicos. Se invalida: log e mantem router anterior.
- **Throttling:** Debounce 500ms; min reload interval 1s.
- **Build:** DISCOVERY_ENABLED (default 1 full build, 0 edge). Dependencias: libcurl4-openssl-dev, libyaml-cpp-dev.

## Dependencias e Integracoes

- **Internas:** ProviderRouter, Config
- **Externas:** Docker API, Kubernetes API, libcurl, libyaml-cpp
- **Arquivos:** dynamic_config.h, config_validator, configuration_watcher, file_provider, docker_provider, kubernetes_provider

## Criterios de Aceitacao

- [ ] Docker provider descobre containers com labels hermes.*
- [ ] Kubernetes provider descobre Services com annotations hermes.io/*
- [ ] File provider vigia YAML/JSON em paths configurados
- [ ] Merge de fragmentos respeita merge_priority
- [ ] Validacao pre-swap impede config invalida
- [ ] Hot reload sem downtime (atomic swap)
- [ ] Endpoints /admin/discovery e /admin/config funcionais

## Riscos e Trade-offs

1. **Dependencias:** libcurl e libyaml-cpp necessarios para discovery.
2. **Polling:** Overhead de polling; intervalos configuraveis.
3. **Validacao:** Config invalida mantem estado anterior; log de erro para diagnostico.
4. **Escalabilidade horizontal:** Multiplas instancias leem mesmas fontes; stateless; sem coordenacao.

## Status de Implementacao

IMPLEMENTADO — Docker, Kubernetes e File providers; ConfigurationWatcher com merge, debounce, validacao pre-swap; hot reload; endpoints admin. Quando DISCOVERY_ENABLED=0, gateway funciona com config estatico.

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
