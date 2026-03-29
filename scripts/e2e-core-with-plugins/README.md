# E2E Tests: Core + All Plugins (Google Test)

Testes de ponta a ponta que validam o HERMES do **básico ao avançado** com **todos os plugins ativos**, verificando que estão registrados e que as APIs respondem corretamente.

## Pré-requisitos comuns

1. **Gateway em execução** com config que habilita todos os plugins (use `config.e2e.json` ou merge no seu `config.json`).
2. **Ollama** (ou backend compatível) acessível na URL configurada (ex.: `http://localhost:11434`).
3. **Modelo nos testes:** os testes de chat/completions usam o modelo **`gemma3:1b`**. No Ollama: `ollama pull gemma3:1b`.

### Usando docker-compose

Se o gateway sobe com o `docker-compose.yml` da raiz do projeto:

- **GATEWAY_URL:** `http://localhost:8080` (porta mapeada no compose).
- **ADMIN_KEY:** use o mesmo valor do compose, ex.: `your-admin-secret` (variável `ADMIN_KEY` no `docker-compose.yml`).
- **Ollama:** deve estar acessível pelo host (ex.: `localhost:11434`). No compose o gateway usa `OLLAMA_URL=http://host.docker.internal:11434` dentro do container.

Exemplo para rodar os testes com o compose em pé:

```bash
GATEWAY_URL=http://localhost:8080 ADMIN_KEY=your-admin-secret ./e2e_core_plugins_test
```

---

## Opção 1: WSL / Linux (Make)

Use esta opção no **WSL** (Ubuntu/Debian) ou em **Linux** nativo.

### Dependências

```bash
sudo apt-get update
sudo apt-get install -y build-essential libgtest-dev libjsoncpp-dev
```

Em algumas distros é preciso compilar a lib gtest:

```bash
cd /usr/src/gtest && sudo cmake . && sudo make && sudo cp lib/*.a /usr/lib
```

### Compilar

Na **raiz do projeto**:

```bash
make e2e-test
```

Isso gera o binário `scripts/e2e-core-with-plugins/e2e_core_plugins_test`.

### Rodar

```bash
# Gateway e Ollama já devem estar rodando
./scripts/e2e-core-with-plugins/e2e_core_plugins_test

# Com URL e admin key
GATEWAY_URL=http://localhost:8080 ADMIN_KEY=seu-admin-key ./scripts/e2e-core-with-plugins/e2e_core_plugins_test
```

---

## Opção 2: Windows (CMake)

Use esta opção no **Windows** quando não houver `make` ou quando quiser que o CMake baixe o Google Test automaticamente (FetchContent).

### Dependências

- **CMake** 3.14+ ([cmake.org](https://cmake.org/download/) ou `winget install Kitware.CMake`)
- **Git** (para o FetchContent baixar o googletest)
- **Compilador C++** (Visual Studio Build Tools ou MinGW)
- **JsonCpp**: instale via vcpkg (`vcpkg install jsoncpp`) ou use o include/lib do seu ambiente e informe `JSONCPP_INCLUDE_DIR` e `JSONCPP_LIBRARY` ao CMake, se necessário.

### Compilar

Na **raiz do projeto** (PowerShell ou cmd):

```powershell
cd D:\workspace\pipeleap\llm-gateway

# Configurar e compilar (CMake baixa o googletest na primeira vez)
cmake -B build-e2e -S scripts/e2e-core-with-plugins
cmake --build build-e2e
```

Se usar **vcpkg** (toolchain):

```powershell
cmake -B build-e2e -S scripts/e2e-core-with-plugins -DCMAKE_TOOLCHAIN_FILE=[caminho-do-vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build build-e2e
```

O executável fica em `build-e2e\Debug\e2e_core_plugins_test.exe` (ou `build-e2e\e2e_core_plugins_test.exe` em geradores single-config).

### Rodar

```powershell
# Gateway e Ollama já devem estar rodando
.\build-e2e\Debug\e2e_core_plugins_test.exe

# Com variáveis de ambiente
$env:GATEWAY_URL="http://localhost:8080"; $env:ADMIN_KEY="seu-admin-key"; .\build-e2e\Debug\e2e_core_plugins_test.exe
```

---

## Variáveis de ambiente

| Variável     | Descrição                          | Default              |
|-------------|-------------------------------------|----------------------|
| `GATEWAY_URL` | URL base do gateway                 | `http://localhost:8080` |
| `ADMIN_KEY`   | Chave admin; se vazia, testes de admin são ignorados (SKIP) | (vazio) |

## O que os testes cobrem

- **Básico:** Health (`/health`), lista de plugins (`/admin/plugins`), métricas com chave `plugins` (`/metrics`), listagem de modelos (`/v1/models`).
- **Intermediário:** Chat completions, text completions e embeddings (`POST /v1/chat/completions`, `/v1/completions`, `/v1/embeddings`) com corpo JSON válido.
- **Avançado:** Streaming SSE, headers de cache (X-Cache), endpoints admin (keys, updates/check), header de API versioning.

Se o backend (Ollama) ou um modelo não estiver disponível, os testes que dependem deles usam `GTEST_SKIP()` em vez de falhar.
