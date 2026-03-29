# Módulo 16: Billing e Faturamento

> **RF-00-FUNCIONAL** — placeholder para implementação pelo produto.

## Descrição

Demonstrativo mensal com itens de cobrança, status de faturas (pendente/paga/atrasada/cancelada), histórico de pagamentos e gestão de métodos de pagamento (cartão/boleto).

## Estrutura esperada

```
src/features/billing/
├── index.ts
├── components/        # BillingPage, InvoiceList, InvoiceDetail, PaymentMethodList
├── hooks/             # useInvoices, usePaymentMethods, useCurrentBill
├── services/          # billing.api.ts
├── types.ts           # Invoice, BillingStatement, BillingItem, PaymentMethod
└── README.md
```

## Referências

- Spec: `RF-00-FUNCIONAL.md` — Módulo 16
- Status de Fatura: `RF-00-FUNCIONAL.md` §16 (tabela de status e cores de Chip)
