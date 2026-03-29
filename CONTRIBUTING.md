# Contributing to HERMES (llm-gateway)

HERMES is the LLM proxy gateway of Pipeleap — a high-performance OpenAI-compatible
reverse proxy written in C++23 with a plugin pipeline system.

For general rules (DCO sign-off, SPDX headers, code of conduct, PR process) see the
[root CONTRIBUTING.md](../CONTRIBUTING.md).

---

## License

The HERMES core is licensed under **Apache 2.0** (see [LICENSE](LICENSE)).

**Enterprise plugins are NOT open-core:**
`src/plugins/enterprise/` is proprietary and governed by the Pipeleap Enterprise
License. Do not submit PRs that touch this directory.

---

## Architecture overview

```
HTTP request
    │
    ▼
IAuth → IRateLimiter
    │
    ▼
PluginManager::run_before_request()   ← plugin pipeline
    │
    ▼
ICache → IRequestQueue → Backend provider (Ollama / OpenAI / OpenRouter)
    │
    ▼
PluginManager::run_after_response()   ← plugin pipeline (reverse order)
    │
    ▼
HTTP response
```

Key source files:

| File | Responsibility |
|---|---|
| `src/gateway.cpp` | HTTP server, route handlers, plugin wiring |
| `src/plugin.h` / `src/plugin.cpp` | `Plugin` base class and `PluginManager` |
| `src/tenant_ctx.h` | Multi-tenant context helpers (`ctx_tenant`, `ctx_app`, `ctx_client`) |
| `src/core_services.cpp` | `IAuth`, `ICache`, `IRateLimiter`, `IRequestQueue` |
| `src/plugins/` | Built-in open-core plugins |
| `src/discovery/` | Config watcher and provider discovery |
| `docs/spec/` | Functional specs (RF-NN-FUNCIONAL.md) |
| `docs/ARCHITECTURE.md` | System architecture document |

---

## Open-core scope

| Path | Open for contribution |
|---|---|
| `src/` (excluding `src/plugins/enterprise/`) | Yes |
| `src/plugins/enterprise/` | No — proprietary |
| `hermes-dashboard/` | Yes |
| `docs/spec/` | Yes (specs for open-core features only) |
| `scripts/` | Yes |

---

## Build

Requirements: `g++` with C++23 support, `libjsoncpp-dev`, `libssl-dev`,
`libcurl4-openssl-dev`, `libyaml-cpp-dev`.

```bash
# Release
make

# Debug (with symbols)
make debug

# Address sanitizer (for PRs touching memory management)
make asan

# Thread sanitizer (for PRs touching concurrency)
make tsan

# Build the edge variant (null implementations)
make edge

# Run locally
make run
```

Docker:
```bash
docker compose up -d --build

# Edge image
docker build -f Dockerfile.edge -t hermes:edge .
```

---

## Testing

Always run the relevant test suite before opening a PR.

```bash
# Functional test against a running gateway
bash scripts/test/test_gateway.sh

# Stress test
bash scripts/stress/stress_test.sh

# Benchmark
bash scripts/benchmark/benchmark_sync.sh
```

**C++ unit tests** (security plugins and core):
```bash
cmake -B build -S scripts/unit-tests
cmake --build build
./build/test_security_plugins
```

**E2E tests** (core with plugin pipeline):
```bash
cmake -B build-e2e -S scripts/e2e-core-with-plugins
cmake --build build-e2e
./build-e2e/e2e_core_plugins_test
```

**Dashboard** (React/Vite):
```bash
cd hermes-dashboard
npm install
npm run lint
npm test
npm run build
```

New behavior must be covered by tests. PRs without tests are not merged.

---

## Plugin development

Third-party plugins must implement the `Plugin` interface from `src/plugin.h`:

```cpp
// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Pipeleap

#include "plugin.h"

class MyPlugin : public Plugin {
public:
    std::string name()    const override { return "my_plugin"; }
    std::string version() const override { return "1.0.0"; }

    bool init(const Json::Value& config) override { /* ... */ return true; }
    PluginResult before_request(Json::Value& body, PluginRequestContext& ctx) override;
    PluginResult after_response(Json::Value& resp,  PluginRequestContext& ctx) override;
    void shutdown() override {}
    Json::Value stats() const override { return {}; }
};
```

Enable via `config.json`:
```json
{
  "plugins": {
    "pipeline": [
      { "name": "my_plugin", "enabled": true, "config": {} }
    ]
  }
}
```

See the full guide in [`plugins/PLUGIN_GUIDE.md`](../plugins/PLUGIN_GUIDE.md).

---

## Multi-tenant context

All plugins receive `PluginRequestContext`. Use helpers from `src/tenant_ctx.h`
to access tenant identity — never parse headers directly inside a plugin:

```cpp
#include "tenant_ctx.h"

std::string tenant = ctx_tenant(ctx);   // "default" if header absent
std::string app    = ctx_app(ctx);
std::string client = ctx_client(ctx);
std::string rk     = ctx_rate_key(ctx); // "tenant:client_id"
```

---

## Adding a new spec

1. Create `docs/spec/RF-NN-FUNCIONAL.md` following the existing format
2. Reference the spec in `docs/ARCHITECTURE.md` if it changes system boundaries
3. Update `docs/spec/ADR-01-CORE-VS-PLUGIN.md` if the pipeline order changes
4. Open a draft PR for discussion before implementing

---

## C++ rules (HERMES-specific)

Beyond the root standards:

- All plugin implementations must be thread-safe (assume concurrent requests)
- `before_request` must be synchronous and return within the request latency budget
- `after_response` must be synchronous unless explicitly documented as fire-and-forget
- Use `std::shared_mutex` + `std::shared_lock` for read-heavy plugin state
- Plugin `init()` is called once from the main thread — no concurrency required there
- The gateway compiles with `-Wall -Wextra`; PRs must introduce zero new warnings

---

## Dashboard (hermes-dashboard)

The dashboard is a React/Vite/MUI SPA that consumes the gateway's `/admin/*` API.

Key conventions:
- Data fetching: `useSWR` via `src/lib/fetcher.ts` and `src/api/gateway.ts`
- Auth: `sessionStorage('hermes_admin_key')` → `Authorization: Bearer <key>`
- Base URL: always prefix with `import.meta.env.VITE_API_URL ?? ''`
- New admin pages follow the pattern in `src/pages/guardrails/index.tsx`
- New routes: add a lazy import in `src/routes/MainRoutes.tsx`
- New nav items: add to the appropriate group in `src/menu-items/dashboard.ts`

---

## CI

GitHub Actions runs on every PR (`.github/workflows/`):

| Workflow | What it checks |
|---|---|
| `build` | Release build, zero warnings |
| `asan` | Address sanitizer (memory safety) |
| `tsan` | Thread sanitizer (data races) |
| `edge` | Edge build with null implementations |
| `docker` | `docker build` of the main image |
| `docker-edge` | `docker build -f Dockerfile.edge` |

All workflows must be green before a PR can be merged.
