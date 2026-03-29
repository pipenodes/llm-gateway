/// <reference types="vite/client" />

// env.d.ts — Tipagem de variáveis de ambiente do Vite
// Declare aqui todas as variáveis VITE_* usadas no projeto.
// Vite expõe apenas variáveis com prefixo VITE_ via import.meta.env.

interface ImportMetaEnv {
  /** URL base da API REST */
  readonly VITE_API_URL: string;
  /** Nome exibido na aplicação */
  readonly VITE_APP_NAME: string;
  /** Versão da aplicação (injetada no build) */
  readonly VITE_APP_VERSION: string;
  /** DSN do Sentry para rastreamento de erros (opcional) */
  readonly VITE_SENTRY_DSN?: string;
  /** URL da página de status do sistema */
  readonly VITE_STATUS_URL?: string;
  /** E-mail de suporte configurável (usado em Forbidden, ServerError) */
  readonly VITE_SUPPORT_EMAIL?: string;
  /** Feature flags — formato JSON stringificado ex: '{"darkMode":true}' */
  readonly VITE_FEATURE_FLAGS?: string;
}

interface ImportMeta {
  readonly env: ImportMetaEnv;
}
