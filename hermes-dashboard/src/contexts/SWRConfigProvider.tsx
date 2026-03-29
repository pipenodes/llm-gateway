import type { ReactNode } from 'react';
import { SWRConfig } from 'swr';

import { fetcher, ApiError } from 'lib/fetcher';
import { useSnackbar } from 'contexts/SnackbarContext';

// SWRConfigProvider — configuração global do SWR
// RF-00-UX-DEFAULT-FOUNDATION — requisições com feedback integrado
//
// - Usa o fetcher base (com auth + 401 redirect)
// - Erros de rede/API disparam toast via SnackbarContext
// - 401 redireciona para /session-expired silenciosamente (sem toast)
// - Retry: 3 tentativas com backoff exponencial (padrão SWR)
// - Revalidação ao focar a janela habilitada (padrão SWR)

interface SWRConfigProviderProps {
  children: ReactNode;
}

export function SWRConfigProvider({ children }: SWRConfigProviderProps) {
  const { showError } = useSnackbar();

  return (
    <SWRConfig
      value={{
        fetcher,
        // Não mostrar toast para 401 — o fetcher já redireciona para /session-expired
        onError: (error: unknown) => {
          if (error instanceof ApiError) {
            if (error.status === 401) return;
            if (error.status === 404) return;
            const msg = error.status >= 500
              ? 'Erro no servidor. Tente novamente em instantes.'
              : error.message;
            showError(msg);
          } else {
            showError('Erro de conexao. Verifique sua internet.');
          }
        },
        // Revalidar ao reconectar (ex: usuário voltou do offline)
        revalidateOnReconnect: true,
        // Não revalidar automaticamente ao focar (evita requisições desnecessárias)
        revalidateOnFocus: false,
        // Intervalo de deduplicação de 5s
        dedupingInterval: 5000,
        // Exibir dados em cache enquanto revalida (stale-while-revalidate)
        keepPreviousData: true
      }}
    >
      {children}
    </SWRConfig>
  );
}
