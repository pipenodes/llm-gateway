# Módulo 3: Templates de E-mail (HTML)

> **RF-00-FUNCIONAL** — placeholder para implementação pelo produto.

## Descrição

Templates HTML transacionais: convite de membro, reset de senha, compartilhamento de artefato. Estrutura Handlebars/Mustache para interpolação de variáveis.

## Estrutura esperada

```
src/features/email-templates/
├── index.ts
├── templates/         # base.html, invite-member.html, reset-password.html, share-link.html
├── components/        # EmailPreview (visualização in-app)
├── services/          # emailTemplates.api.ts — renderização server-side
├── types.ts           # EmailTemplate, EmailVariables
└── README.md
```

## Referências

- Spec: `RF-00-FUNCIONAL.md` — Módulo 3
