# Benchmark: comparação de performance entre modelos (Ollama)

Suite de testes **gtest** que compara latência e sucesso das respostas entre **gemma3:1b**, **gemma3:4b** e **gemma3:12b** via provider Ollama (através do HERMES).

## Dois executáveis

- **benchmark_models_test** — teste rápido: latência média por modelo e (assunto, complexidade); sem relatórios.
- **benchmark_suite_test** — suite completa: métricas (TTFT, latency, tokens/s, tokens in/out), scoring (exact match, F1, BLEU, math correct, coerência/raciocínio) e geração de relatórios. Nomes dos arquivos incluem o slug dos modelos (ex.: **report-gemma3_4b-20260226-160930.json**). Todos os relatórios trazem **report_id** e **execution_date**. Arquivos: JSON, CSV, TXT (Q&A), **report_questions_and_answers.txt** (cópia do último run) e **report-{slug}-{id}.md** (análise pelo gemma3:12b).

## Estrutura da suite (benchmark_suite_test)

```
scripts/benchmark-models/
  metrics/           # TTFT, latency, token counter, resource (stub)
  scoring/           # expected_answers, math_validator, logic_validator, score_utils (exact match, F1, BLEU)
  prompts.h/cpp      # 6 assuntos × 3 níveis × 10 perguntas
  benchmark_suite_test.cpp
```

## Métricas e indicadores

- **Performance:** TTFT (Time To First Token), latência total, tokens/s, tokens de entrada/saída; RAM/VRAM/CPU/GPU (stub; não expostos pela API).
- **Qualidade objetiva:** exact match, F1, BLEU, math correct (onde há resposta esperada).
- **Qualidade subjetiva/heurística:** coerência estrutural, raciocínio passo a passo, aderência ao prompt.
- **Resumo no JSON:** cost_per_correct (tempo por resposta correta), avg_latency_ms, avg_ttft_ms.

## Categorias

- **Assuntos:** geral, matemática, raciocínio lógico, Python, JavaScript, C#
- **Complexidade dos prompts:** básico, médio, avançado
- **10 perguntas** por (assunto, complexidade) — total **180 perguntas** por modelo (540 chamadas no benchmark completo)

As perguntas são inspiradas em benchmarks de mercado (MMLU, GSM8K, HumanEval, MBPP, etc.).

## Pré-requisitos

1. **Gateway** em execução (ex.: `docker compose up -d hermes`).
2. **Ollama** acessível na URL configurada no gateway (ex.: `OLLAMA_URL=http://host.docker.internal:11434`).
3. Modelos no Ollama: `ollama pull gemma3:1b`, `ollama pull gemma3:4b`, `ollama pull gemma3:12b`.

## Config (sem rebuild)

Na pasta do volume de relatórios (ex.: **`benchmark-reports/`** no Docker) use **`benchmark_config.json`** para definir provider e modelos. Assim você só altera o arquivo e roda o teste de novo.

Exemplo `benchmark_config.json`:

```json
{
  "provider": "ollama",
  "models": ["gemma3:4b"],
  "system_prompt": "You are a precise evaluation assistant. Answer concisely.",
  "system_prompts": {
    "general": "You are a knowledge assistant. Answer factually and briefly.",
    "math": "You are a math assistant. End with the final numerical answer.",
    "logical_reasoning": "You are a logic assistant. Reason step by step, then state your conclusion.",
    "python": "You are a Python expert. Provide correct code or precise short answers.",
    "javascript": "You are a JavaScript expert. Provide correct code or precise short answers.",
    "csharp": "You are a C# expert. Provide correct code or precise short answers."
  }
}
```

- **provider:** `ollama` (por enquanto o único usado; reservado para futuras extensões).
- **models:** lista de modelos (ex.: `["gemma3:1b"]`, `["gemma3:4b","gemma3:12b"]`).
- **system_prompt:** (opcional) prompt de sistema usado em todos os testes quando não houver entrada em `system_prompts` para o assunto.
- **system_prompts:** (opcional) objeto com chaves por assunto (`general`, `math`, `logical_reasoning`, `python`, `javascript`, `csharp`). O valor é o system prompt usado nas perguntas daquele assunto. Se o assunto não estiver presente, usa-se `system_prompt`.

Se o arquivo não existir ou estiver inválido, vale **`BENCHMARK_MODELS`** (env, vírgula) ou o default `gemma3:1b,gemma3:4b,gemma3:12b`.

## Variáveis de ambiente

| Variável     | Descrição              | Default              |
|-------------|------------------------|----------------------|
| `GATEWAY_URL` | URL base do gateway    | `http://localhost:8080` |
| `API_KEY`     | Chave de API (se o gateway exigir) | (vazio)        |
| `REPORT_DIR`  | Pasta de relatórios e do config     | `.` (Docker: `/app/reports`) |
| `BENCHMARK_MODELS` | Fallback de modelos (vírgula) se não houver config | `gemma3:1b,gemma3:4b,gemma3:12b` |

## Compilar e rodar

### Opção 1: Docker Compose (recomendado)

Na raiz do projeto, com o gateway já em pé:

```bash
docker compose run --rm benchmark-models
```

O serviço executa **benchmark_suite_test** e gera na pasta **./benchmark-reports/** (bind mount). **Provider e modelos** vêm de **`benchmark-reports/benchmark_config.json`** (edite e rode de novo, sem rebuild).

Arquivos gerados (com **report_id** e **execution_date** em todos):

- **report-{models_slug}-{id}.json** — report_id, execution_date, models, runs + summary.
- **report-{models_slug}-{id}.csv** — linhas `# report_id`, `# execution_date`, `# models`, depois tabela plana.
- **report-{models_slug}-{id}_questions_and_answers.txt** e **report_questions_and_answers.txt** — report_id, execution_date, Models, depois Q&A.
- **report-{models_slug}-{id}.md** — report_id, execution_date, Models, depois análise pelo **gemma3:12b**.

### Opção 2: Linux / WSL (Make)

Na raiz do projeto:

```bash
make benchmark-models   # compila
GATEWAY_URL=http://localhost:8080 API_KEY=your-key ./scripts/benchmark-models/benchmark_models_test
```

Requer: `build-essential`, `libgtest-dev`, `libjsoncpp-dev` (e gtest compilado, se necessário).

### Opção 3: CMake (Windows ou Linux)

Na raiz do projeto:

```bash
cmake -B build-bench -S scripts/benchmark-models
cmake --build build-bench
```

Rodar (PowerShell):

```powershell
$env:GATEWAY_URL="http://localhost:8080"; $env:API_KEY="your-key"; .\build-bench\Debug\benchmark_models_test.exe
```

## Saída

- **RunFullBenchmark:** tabela de latência média (ms) por modelo e por assunto; depois por (assunto, complexidade) para cada modelo.
- **BenchmarkOneModel/*:** um teste por modelo (útil para rodar um modelo só).

Se alguma requisição falhar (gateway/Ollama indisponível ou modelo não encontrado), o teste reporta o número de falhas e pode falhar com `EXPECT_EQ(totalFailures, 0)`.
