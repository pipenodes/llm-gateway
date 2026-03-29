# Módulo 15: Licenciamento e Limites

> **RF-00-FUNCIONAL** — placeholder para implementação pelo produto.

## Descrição

Matriz de planos (Free/Starter/Pro/Enterprise) com features e quotas por plano, indicadores de uso em tempo real (LinearProgress), avisos de limite atingido e comparação de planos para upgrade.

## Estrutura esperada

```
src/features/licensing/
├── index.ts
├── components/        # PlanComparisonTable, QuotaUsageCard, UpgradeDialog, LimitWarning
├── hooks/             # usePlan, useQuotas, useUsage
├── services/          # licensing.api.ts
├── types.ts           # Plan, PlanFeature, PlanQuota, UsageMetric
└── README.md
```

## Referências

- Spec: `RF-00-FUNCIONAL.md` — Módulo 15 (inclui estrutura da Matriz de Planos)
