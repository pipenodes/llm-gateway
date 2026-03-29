# Módulo 8: Ajuda e Onboarding

> **RF-00-FUNCIONAL** — placeholder para implementação pelo produto.

## Descrição

FAQ em Accordion por categoria, ajuda contextual por módulo (Tooltip/Popover com link para doc), empty states com CTA de onboarding e tour guiado para novos usuários.

## Componente shell base

O menu de perfil já expõe o callback `onHelp` via props no Profile — conecte-o a este módulo.

## Estrutura esperada

```
src/features/help/
├── index.ts
├── components/        # HelpDrawer, FAQAccordion, ContextualHelp, OnboardingTour
├── hooks/             # useHelp, useFAQ
├── services/          # help.api.ts
├── types.ts           # FAQItem, HelpArticle, OnboardingStep
└── README.md
```

## Referências

- Spec: `RF-00-FUNCIONAL.md` — Módulo 8
