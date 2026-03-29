# Módulo 2: Compartilhamento de Artefatos

> **RF-00-FUNCIONAL** — placeholder para implementação pelo produto.

## Descrição

Links diretos com permissão configurável (visualização/edição), modos de visibilidade, shortcuts de compartilhamento (e-mail, WhatsApp, copiar link) e expiração automática.

## Estrutura esperada

```
src/features/sharing/
├── index.ts
├── components/        # ShareDialog, ShareLinkInput, PermissionSelect, ExpirationSelect
├── hooks/             # useShareLink, usePermissions
├── services/          # sharing.api.ts
├── types.ts           # ShareLink, SharePermission, ShareExpiration
└── README.md
```

## Referências

- Spec: `RF-00-FUNCIONAL.md` — Módulo 2
