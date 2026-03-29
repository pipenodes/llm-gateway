# Módulo 1: Identidade, Usuários e Colaboração

> **RF-00-FUNCIONAL** — placeholder para implementação pelo produto.

## Descrição

Convite de membros, papéis (RBAC), permissões granulares e estados de usuário (ativo/pendente/suspenso).

## Estrutura esperada

```
src/features/identity/
├── index.ts           # Exportações públicas do módulo
├── components/        # InviteForm, MemberList, RoleSelect, UserStatusChip
├── hooks/             # useMembers, useInvite, useRoles
├── services/          # identity.api.ts — CRUD de membros e convites
├── types.ts           # Member, Invite, Role, Permission
└── README.md
```

## Referências

- Spec: `RF-00-FUNCIONAL.md` — Módulo 1
- Design: `.cursor/rules/71-design-system-mui.mdc` (Dialog, Autocomplete, DataGridPro, DatePicker)
