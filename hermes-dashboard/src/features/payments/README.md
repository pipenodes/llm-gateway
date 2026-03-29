# Módulo 17: Pagamentos (Stripe)

> **RF-00-FUNCIONAL** — placeholder para implementação pelo produto.

## Descrição

Integração com Stripe: PaymentIntent via API, Stripe Elements para checkout, webhooks para eventos de pagamento e PCI DSS compliance via tokenização.

## Segurança

- **NUNCA** armazenar dados de cartão — usar apenas tokens Stripe
- Validar webhooks com `stripe.webhooks.constructEvent` + assinatura secreta
- Usar Stripe.js no frontend (não carregar dados sensíveis no bundle)

## Estrutura esperada

```
src/features/payments/
├── index.ts
├── components/        # CheckoutForm, PaymentMethodForm, PaymentStatus
├── hooks/             # useStripe, usePaymentIntent, useCheckout
├── services/          # payments.api.ts — cria PaymentIntent via backend
├── types.ts           # PaymentIntent, PaymentStatus, StripeWebhookEvent
└── README.md
```

## Referências

- Spec: `RF-00-FUNCIONAL.md` — Módulo 17
- Stripe docs: https://docs.stripe.com/payments
