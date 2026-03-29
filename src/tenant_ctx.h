#pragma once
#include "plugin.h"
#include <string>

// Helpers para acessar o contexto multi-tenant de forma uniforme em plugins enterprise.
// Populados pelo gateway core antes de run_before_request() — ver RF-10-ADENDO-TENANT-CTX.md.

inline std::string ctx_tenant(const PluginRequestContext& ctx) {
    auto it = ctx.metadata.find("tenant_id");
    return (it != ctx.metadata.end() && !it->second.empty()) ? it->second : "default";
}

inline std::string ctx_app(const PluginRequestContext& ctx) {
    auto it = ctx.metadata.find("app_id");
    return (it != ctx.metadata.end() && !it->second.empty()) ? it->second : "default";
}

inline std::string ctx_client(const PluginRequestContext& ctx) {
    auto it = ctx.metadata.find("client_id");
    return (it != ctx.metadata.end() && !it->second.empty()) ? it->second : ctx.key_alias;
}

// Chave composta para rate buckets e agrupamentos: "tenant:app"
inline std::string ctx_tenant_app_key(const PluginRequestContext& ctx) {
    return ctx_tenant(ctx) + ":" + ctx_app(ctx);
}

// Chave para rate limit por entidade de consumo: "tenant:client_id"
inline std::string ctx_rate_key(const PluginRequestContext& ctx) {
    return ctx_tenant(ctx) + ":" + ctx_client(ctx);
}
