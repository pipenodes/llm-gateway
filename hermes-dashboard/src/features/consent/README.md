# Módulo 12: Opt-ins e Opt-outs

> **RF-00-FUNCIONAL** — placeholder para implementação pelo produto.

## Descrição

Consentimento explícito para comunicações (marketing, notificações push, relatórios) e funcionalidades opcionais (analytics, telemetria). Opt-out disponível a qualquer momento com histórico de consentimentos.

## Estrutura esperada

```
src/features/consent/
├── index.ts
├── components/        # ConsentForm, ConsentHistory, ConsentBanner
├── hooks/             # useConsent, useConsentHistory
├── services/          # consent.api.ts
├── types.ts           # ConsentItem, ConsentStatus, ConsentRecord
└── README.md
```

## Referências

- Spec: `RF-00-FUNCIONAL.md` — Módulo 12
