# Módulo 13: Data Privacy (LGPD / GDPR)

> **RF-00-FUNCIONAL** — placeholder para implementação pelo produto.

## Descrição

Visualização dos dados pessoais armazenados, exportação em JSON/CSV, anonimização de dados históricos e solicitação de exclusão de conta (com período de retenção configurável).

## Estrutura esperada

```
src/features/privacy/
├── index.ts
├── components/        # MyDataPage, DataExportButton, DeleteAccountDialog, AnonymizeDialog
├── hooks/             # useMyData, useDataExport
├── services/          # privacy.api.ts
├── types.ts           # PersonalData, DataExportRequest, DeletionRequest
└── README.md
```

## Referências

- Spec: `RF-00-FUNCIONAL.md` — Módulo 13
- Lei: LGPD (Lei 13.709/2018), GDPR (Regulamento UE 2016/679)
