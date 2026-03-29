# Módulo 14: Termos, Licenças e Políticas

> **RF-00-FUNCIONAL** — placeholder para implementação pelo produto.

## Descrição

Aceite explícito de Termos de Uso e Política de Privacidade com versionamento. Reaceite obrigatório quando documentos são atualizados — bloqueia o acesso com Dialog modal até aceite.

## Estrutura esperada

```
src/features/terms/
├── index.ts
├── components/        # TermsAcceptDialog, TermsPage, PolicyVersionHistory
├── hooks/             # useTermsAcceptance, useLatestTerms
├── services/          # terms.api.ts
├── types.ts           # Terms, TermsVersion, AcceptanceRecord
└── README.md
```

## Referências

- Spec: `RF-00-FUNCIONAL.md` — Módulo 14
