# RF-03 — Request Queue com Prioridade

- **RF:** RF-03
- **Titulo:** Request Queue com Prioridade
- **Autor:** HERMES Team
- **Data:** 2026-03-09
- **Versao:** 1.0
- **Status:** IMPLEMENTADO

## Objetivo

Implementar uma fila de requests com suporte a prioridade, permitindo enfileirar requests quando os backends estao sobrecarregados em vez de rejeita-las imediatamente. Inclui controle de concorrencia por backend, timeout de fila e priorizacao por API key.

## Escopo

- **Inclui:** RequestQueue, IRequestQueue, priority (Critical, High, Normal, Low), workers, concurrency semaphore, admission controller, metricas de fila
- **Nao inclui:** Aging de prioridade, cancelamento automatico ao desconectar cliente

## Descricao Funcional Detalhada

### Arquitetura

Requests HTTP -> Admission Controller -> (fila cheia: 429 Queue Full) | (aceita: Priority Queue) -> Worker Threads -> Backend (Ollama/Provider). Concurrency Limiter (Semaphore max N) controla requests simultaneas ao backend.

### Componentes

- **RequestQueue:** Fila de prioridade thread-safe com timeout.
- **ConcurrencyLimiter:** Semaforo que limita requests simultaneas ao backend.
- **QueuedRequest:** Estrutura que encapsula a request com prioridade e callback.

### Cenarios

1. **Picos de carga:** Deploy gera muitas requests simultaneas. Com fila, sao processadas sequencialmente em vez de falharem.
2. **Modelos grandes:** Modelos pesados (70B) processam 1 request por vez. Sem fila, so a primeira tem sucesso.
3. **Prioridade:** Requests de producao tem prioridade sobre desenvolvimento.
4. **Experiencia do usuario:** Esperar 30s e melhor que receber erro 502.

## Interface / Contrato

```cpp
enum class Priority : int {
    Critical = 0,
    High = 1,
    Normal = 2,
    Low = 3
};

struct QueuedRequest {
    Priority priority;
    std::string request_id;
    std::string key_alias;
    std::chrono::steady_clock::time_point enqueued_at;
    std::chrono::milliseconds timeout;
    std::function<void()> execute;
    std::promise<bool> completion;
};

class RequestQueue {
public:
    explicit RequestQueue(size_t max_size = 1000, size_t max_concurrency = 4) noexcept;
    struct EnqueueResult {
        bool accepted;
        size_t position;
        std::future<bool> completion;
    };
    [[nodiscard]] EnqueueResult enqueue(Priority priority,
                                         std::chrono::milliseconds timeout,
                                         std::function<void()> task,
                                         const std::string& request_id = "",
                                         const std::string& key_alias = "");
    [[nodiscard]] size_t pending() const noexcept;
    [[nodiscard]] size_t active() const noexcept;
    [[nodiscard]] Json::Value stats() const;
    void shutdown();
};
```

## Configuracao

### config.json

```json
{
  "queue": {
    "enabled": true,
    "max_size": 1000,
    "max_concurrency": 4,
    "default_timeout_ms": 60000,
    "worker_threads": 4
  }
}
```

### Variaveis de Ambiente

| Variavel | Descricao | Default |
|----------|-----------|---------|
| `QUEUE_ENABLED` | Habilitar fila de requests | `false` |
| `QUEUE_MAX_SIZE` | Tamanho maximo da fila | `1000` |
| `QUEUE_MAX_CONCURRENCY` | Requests simultaneas ao backend | `4` |
| `QUEUE_TIMEOUT_MS` | Timeout padrao na fila | `60000` |
| `QUEUE_WORKERS` | Threads de worker | `4` |

## Endpoints

| Metodo | Path | Descricao |
|--------|------|-----------|
| `GET` | `/admin/queue` | Status da fila (pendentes, ativos, metricas) |
| `DELETE` | `/admin/queue` | Limpar fila (cancelar requests pendentes) |

### Response de `/admin/queue`

```json
{
  "pending": 12,
  "active": 4,
  "max_size": 1000,
  "max_concurrency": 4,
  "metrics": {
    "total_enqueued": 5420,
    "total_processed": 5380,
    "total_rejected": 28,
    "total_timed_out": 12,
    "avg_wait_ms": 2340
  }
}
```

## Regras de Negocio

- Fila cheia: admission controller retorna 429 Queue Full.
- Prioridade: Critical > High > Normal > Low; dentro da mesma prioridade, FIFO por enqueued_at.
- Timeout: requests que excedem timeout na fila sao descartadas.
- Prioridade por key: ApiKeyManager pode associar prioridade a cada key.

## Dependencias e Integracoes

- **Internas:** ApiKeyManager (para prioridade por key), MetricsCollector (metricas de fila)
- **Externas:** Nenhuma
- **C++23:** std::counting_semaphore, std::jthread

## Criterios de Aceitacao

- [ ] RequestQueue enfileira requests quando backend ocupado
- [ ] Prioridade determina ordem de processamento
- [ ] Concurrency semaphore limita requests simultaneas ao backend
- [ ] Fila cheia retorna 429 Queue Full
- [ ] Timeout descarta requests que esperam demais
- [ ] GET /admin/queue retorna pending, active e metricas
- [ ] DELETE /admin/queue limpa fila

## Riscos e Trade-offs

1. **Latencia:** Requests enfileiradas tem latencia maior. Cliente precisa lidar com timeouts maiores.
2. **Memoria:** Cada request na fila consome memoria. Limitar max_size para evitar OOM.
3. **Streaming:** Requests de streaming na fila sao complexas. Conexao SSE precisa ficar aberta enquanto espera.
4. **Fairness:** Prioridade muito agressiva pode starvar requests de baixa prioridade. Considerar aging.
5. **Timeout do cliente:** Se cliente tem timeout 30s e fila demora 45s, conexao cai antes. Header Retry-After ajuda.
6. **Cancelamento:** Se cliente desconecta, request na fila deveria ser cancelada (futuro).

## Status de Implementacao

IMPLEMENTADO — RequestQueue com prioridade, workers, concurrency semaphore e IRequestQueue implementados. Endpoints /admin/queue disponiveis.

## Checklist de Qualidade

- [ ] Objetivo claro e testavel
- [ ] Escopo dentro/fora definido
- [ ] Regras de negocio sem ambiguidade
- [ ] Criterios de aceitacao verificaveis
- [ ] Excecoes e limites cobertos
