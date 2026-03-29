#pragma once
// Compile-time feature flags. All can be overridden via -D in Makefile.
// The open-core model uses ENTERPRISE_ENABLED to gate premium plugins.

#ifndef BENCHMARK_ENABLED
#define BENCHMARK_ENABLED 1
#endif
#ifndef DOCS_ENABLED
#define DOCS_ENABLED 1
#endif
#ifndef MULTI_PROVIDER
#define MULTI_PROVIDER 1
#endif
#ifndef EDGE_CORE
#define EDGE_CORE 0
#endif
#ifndef DISCOVERY_ENABLED
#define DISCOVERY_ENABLED 1
#endif

// ── Open-core gate ────────────────────────────────────────────────────────────
// ENTERPRISE_ENABLED=1  → enterprise plugins are loaded (static or dynamic).
// ENTERPRISE_ENABLED=0  → core-only OSS binary; enterprise plugins are skipped.
//
// For dynamic enterprise loading at runtime (marketplace hot-reload),
// set ENTERPRISE_ENABLED=1 and provide PIPELEAP_ENTERPRISE_PLUGIN_DIR env var.
#ifndef ENTERPRISE_ENABLED
#define ENTERPRISE_ENABLED 1
#endif

// Directory scanned for enterprise .so plugins at startup.
// Can be overridden at runtime via PIPELEAP_ENTERPRISE_PLUGIN_DIR env var.
#ifndef ENTERPRISE_PLUGIN_DIR
#define ENTERPRISE_PLUGIN_DIR "plugins/enterprise"
#endif
