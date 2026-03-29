# Módulo 11: Segurança e Sessões

> **RF-00-FUNCIONAL** — placeholder para implementação pelo produto.

## Descrição

Lista de sessões ativas (dispositivo, IP, localização), encerramento remoto de sessão, reautenticação para ações sensíveis e configuração de MFA.

## Componente shell base

A tela de sessão expirada já está implementada:
`src/pages/errors/SessionExpired.tsx`

## Estrutura esperada

```
src/features/security/
├── index.ts
├── components/        # ActiveSessionsList, SessionItem, ReauthDialog, MFASetup
├── hooks/             # useSessions, useReauth, useMFA
├── services/          # security.api.ts
├── types.ts           # Session, MFAMethod, ReauthResult
└── README.md
```

## Referências

- Spec: `RF-00-FUNCIONAL.md` — Módulo 11
