# Módulo 6: Notificações e Atividades

> **RF-00-FUNCIONAL** — placeholder para implementação pelo produto.

## Descrição

Feed de notificações em tempo real (WebSocket/SSE), marcar como lida (individual/todas), histórico paginado e integração com o badge do sino no header.

## Componente shell base

O componente de shell já está implementado:
`src/layout/Dashboard/Header/HeaderContent/Notification.tsx`

Ele aceita `items`, `unreadCount`, `onMarkAllRead`, `error` e `onRetry` via props — conecte à API real via SWR/React Query neste módulo.

## Estrutura esperada

```
src/features/notifications/
├── index.ts
├── components/        # NotificationItem, NotificationBadge, NotificationSettings
├── hooks/             # useNotifications, useNotificationSocket
├── services/          # notifications.api.ts
├── types.ts           # Notification, NotificationCategory, NotificationStatus
└── README.md
```

## Referências

- Spec: `RF-00-FUNCIONAL.md` — Módulo 6
