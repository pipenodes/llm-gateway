# Scripts HERMES

Scripts organizados por assunto.

## Estrutura

```
scripts/
├── benchmark/     # Benchmark de modelos LLM (Feature 11)
│   ├── benchmark_sync.sh
│   ├── benchmark_runner.py
│   ├── benchmark_prompts.json
│   └── benchmark_tools.json
├── stress/       # Testes de carga
│   └── stress_test.sh
├── test/         # Testes funcionais (compatibilidade OpenAI)
│   └── test_gateway.sh
├── e2e-core-with-plugins/  # E2E com Google Test (core + todos os plugins)
│   ├── e2e_core_plugins_test.cpp
│   ├── config.e2e.json
│   └── README.md
└── docker/       # Orquestração Docker
    └── run_with_docker.sh
```

## Uso

Execute sempre a partir da **raiz do projeto**:

```bash
# Testes funcionais
bash scripts/test/test_gateway.sh http://localhost:8080

# E2E (Google Test) - gateway deve estar rodando com config.e2e.json
make e2e-test && ./scripts/e2e-core-with-plugins/e2e_core_plugins_test

# Stress test (256 requests, 24 concorrentes)
bash scripts/stress/stress_test.sh http://localhost:8080 256 24

# Benchmark
bash scripts/benchmark/benchmark_sync.sh http://localhost:8080 _ full
bash scripts/benchmark/benchmark_sync.sh http://localhost:8080 _ basic
bash scripts/benchmark/benchmark_sync.sh http://localhost:8080 _ general_basic

# Docker Compose
bash scripts/docker/run_with_docker.sh
```

## Saídas

- **Benchmark**: arquivos `.json`, `.md`, `.csv` na raiz do projeto
- **Stress test**: métricas no stdout
- **Test**: PASS/FAIL no stdout
