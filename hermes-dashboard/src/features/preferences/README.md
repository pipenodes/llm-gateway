# Módulo 10: Preferências Globais

> **RF-00-FUNCIONAL** — placeholder para implementação pelo produto.

## Descrição

Tema (claro/escuro/sistema), idioma e atalhos de teclado configuráveis. Preferências persistidas no localStorage e sincronizadas com o servidor do usuário.

## Componentes shell base

- ThemeToggle: `src/layout/Dashboard/Header/HeaderContent/Profile/ThemeToggle.tsx`
- LanguageSwitcher: `src/layout/Dashboard/Header/HeaderContent/Profile/LanguageSwitcher.tsx`

## Estrutura esperada

```
src/features/preferences/
├── index.ts
├── components/        # PreferencesPage, KeyboardShortcuts, NotificationPreferences
├── hooks/             # usePreferences, useKeyboardShortcuts
├── services/          # preferences.api.ts — sync com servidor
├── types.ts           # UserPreferences, ThemeMode, Language
└── README.md
```

## Referências

- Spec: `RF-00-FUNCIONAL.md` — Módulo 10
