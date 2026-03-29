# Módulo 7: Suporte e Atendimento

> **RF-00-FUNCIONAL** — placeholder para implementação pelo produto.

## Descrição

Abertura de ticket com categoria (bug/dúvida/feature) e prioridade (baixa/média/alta/crítica), acompanhamento de status em tempo real, histórico de conversas e avaliação de atendimento.

## Estrutura esperada

```
src/features/support/
├── index.ts
├── components/        # NewTicketForm, TicketList, TicketDetail, TicketStatusChip
├── hooks/             # useTickets, useTicketDetail
├── services/          # support.api.ts
├── types.ts           # Ticket, TicketStatus, TicketPriority, TicketCategory
└── README.md
```

## Referências

- Spec: `RF-00-FUNCIONAL.md` — Módulo 7
