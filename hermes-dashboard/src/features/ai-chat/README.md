# Módulo 5: Chat com IA (Serviço Transversal)

> **RF-00-FUNCIONAL** — placeholder para implementação pelo produto.

## Descrição

Drawer lateral persistente com histórico de conversas, suporte a streaming de respostas, estados visuais (digitando, erro, carregando) e ações rápidas por mensagem.

## Estrutura esperada

```
src/features/ai-chat/
├── index.ts
├── components/        # ChatDrawer, MessageList, MessageBubble, ChatInput, TypingIndicator
├── hooks/             # useChat, useChatHistory, useStreamingResponse
├── services/          # aiChat.api.ts — SSE/WebSocket para streaming
├── types.ts           # ChatMessage, ChatSession, StreamingState
└── README.md
```

## Referências

- Spec: `RF-00-FUNCIONAL.md` — Módulo 5
