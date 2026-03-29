# RF-02 — Usage Tracking e Quotas

- **RF:** RF-02
- **Titulo:** Usage Tracking e Quotas
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Implementar rastreamento detalhado de uso (tokens consumidos, requests realizadas) por API key, com suporte a quotas configuraveis (limites diarios, mensais) e persistencia em disco. Permite controlar o consumo de recursos e habilitar cenarios de billing.

## Escopo

- **Inclui:** UsageTracker, UsageRecord, QuotaPolicy, per-key tracking, contadores diarios/mensais, persistencia em usage.json, endpoints admin de uso
- **Nao inclui:** Billing externo, alertas de consumo (80% quota), integracao com sistemas de cobranca

## Descricao Funcional Detalhada

### Core vs Plugin

- Se a **quota bloquear** a request no path (ex.: 429 ao exceder limite), faz sentido interface no core `IUsageTracker` e implementacao plugavel (como IAuth/IRateLimiter), com checagem antes de enviar ao backend.
- Se for apenas **contabilidade** pos-request (relatorios, billing), pode ser um plugin que consome o evento "request completed" e atualiza contadores; sem necessidade de interface no core para decisao de aceitar/rejeitar.

### Arquitetura

Request HTTP -> HERMES -> Handler -> Backend -> Response -> UsageTracker -> Contadores em Memoria / usage.json (periodico). QuotaCheck determina se entrega response ou retorna 429 Quota Exceeded.

### Componentes

- **UsageTracker:** Classe principal que mantem contadores por key.
- **UsageRecord:** Registro de uso por key por periodo.
- **QuotaPolicy:** Configuracao de limites por key ou global.

### Cenarios

1. **Ambiente compartilhado:** 10 desenvolvedores compartilham o gateway. Tracking permite saber quem consome mais.
2. **Quotas de uso:** Limitar key de teste a 10.000 tokens/dia para evitar abuso.
3. **Billing interno:** Cobrar departamentos pelo uso real de tokens.
4. **Alertas de consumo:** (futuro) Notificar quando key atinge 80% da quota.

## Interface / Contrato

```cpp
struct UsageRecord {
    int64_t prompt_tokens = 0;
    int64_t completion_tokens = 0;
    int64_t total_tokens = 0;
    int64_t request_count = 0;
    int64_t period_start = 0;  // epoch seconds
};

struct QuotaPolicy {
    int64_t max_tokens_per_day = 0;     // 0 = ilimitado
    int64_t max_tokens_per_month = 0;
    int64_t max_requests_per_day = 0;
    int64_t max_requests_per_month = 0;
};

class UsageTracker {
public:
    void init(const std::string& path);
    void record(const std::string& key_alias, const std::string& model,
                int prompt_tokens, int completion_tokens);
    [[nodiscard]] bool check_quota(const std::string& key_alias,
                                   const QuotaPolicy& policy) const;
    [[nodiscard]] UsageRecord get_daily(const std::string& key_alias) const;
    [[nodiscard]] UsageRecord get_monthly(const std::string& key_alias) const;
    [[nodiscard]] Json::Value get_usage_json(const std::string& key_alias) const;
    [[nodiscard]] Json::Value get_all_usage_json() const;
    void flush();
};
```

## Configuracao

### config.json

```json
{
  "usage": {
    "enabled": true,
    "persist_path": "usage.json",
    "flush_interval_seconds": 60,
    "default_quota": {
      "max_tokens_per_day": 0,
      "max_tokens_per_month": 0,
      "max_requests_per_day": 0,
      "max_requests_per_month": 0
    }
  }
}
```

### Variaveis de Ambiente

| Variavel | Descricao | Default |
|----------|-----------|---------|
| `USAGE_ENABLED` | Habilitar tracking de uso | `false` |
| `USAGE_FLUSH_INTERVAL` | Intervalo de flush em segundos | `60` |
| `DEFAULT_DAILY_TOKEN_QUOTA` | Quota diaria padrao de tokens | `0` (ilimitado) |
| `DEFAULT_MONTHLY_TOKEN_QUOTA` | Quota mensal padrao de tokens | `0` (ilimitado) |

## Endpoints

| Metodo | Path | Descricao |
|--------|------|-----------|
| `GET` | `/admin/usage` | Uso agregado de todas as keys |
| `GET` | `/admin/usage/{alias}` | Uso detalhado de uma key |
| `DELETE` | `/admin/usage/{alias}` | Reset dos contadores de uma key |

### Response de `/admin/usage/{alias}`

```json
{
  "alias": "dev-team",
  "daily": {
    "prompt_tokens": 15230,
    "completion_tokens": 8920,
    "total_tokens": 24150,
    "request_count": 142,
    "period_start": 1740355200
  },
  "monthly": {
    "prompt_tokens": 482100,
    "completion_tokens": 231050,
    "total_tokens": 713150,
    "request_count": 4230,
    "period_start": 1738368000
  },
  "by_model": {
    "llama3:8b": { "total_tokens": 512000, "request_count": 3100 },
    "gpt-4o": { "total_tokens": 201150, "request_count": 1130 }
  },
  "quota": {
    "max_tokens_per_day": 100000,
    "max_tokens_per_month": 2000000,
    "used_percentage_daily": 24.15,
    "used_percentage_monthly": 35.66
  }
}
```

## Regras de Negocio

- Quota 0 significa ilimitado.
- Rotacao de periodos: detectar mudanca de dia/mes UTC para rotacionar contadores.
- Persistencia: flush periodico; aceitar perda de ate flush_interval segundos em crash.
- Quota check antes vs depois: checar antes evita desperdicio mas nao sabe tokens exatos; checar depois e mais preciso mas permite estourar pontualmente.

## Dependencias e Integracoes

- **Internas:** ApiKeyManager (para quotas por key), Logger (para eventos de quota)
- **Externas:** Nenhuma

## Criterios de Aceitacao

- [ ] UsageTracker registra tokens e requests por key
- [ ] Contadores diarios e mensais funcionam com rotacao de periodos
- [ ] check_quota retorna false e request retorna 429 quando quota excedida
- [ ] Persistencia em usage.json com flush periodico
- [ ] Endpoints GET /admin/usage e GET /admin/usage/{alias} retornam dados corretos
- [ ] DELETE /admin/usage/{alias} reseta contadores da key
- [ ] Quotas configuráveis por key e globalmente

## Riscos e Trade-offs

1. **Contencao de lock:** record() chamado em cada request. Usar shared_mutex com write lock rapido.
2. **Rotacao de periodos:** Detectar mudanca de dia/mes UTC sem perder dados.
3. **Persistencia:** Flush periodico pode perder dados em crash. Aceitar perda de ate flush_interval.
4. **Tamanho do arquivo:** usage.json pode crescer com muitas keys/modelos. Considerar compactacao ou rotacao mensal.
5. **Quota check antes vs depois:** Trade-off entre precisao e desperdicio de recursos.

## Status de Implementacao

IMPLEMENTADO — UsageTrackingPlugin implementado com: rastreamento por key (daily/monthly), rotacao de periodos UTC, persistencia em usage.json, flush periodico, quota check com HTTP 429, endpoints GET /admin/usage, GET /admin/usage/{alias}, DELETE /admin/usage/{alias}.

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
