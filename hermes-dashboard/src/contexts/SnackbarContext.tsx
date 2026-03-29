import { createContext, useCallback, useContext, useState } from 'react';
import type { ReactNode } from 'react';

import Alert from '@mui/material/Alert';
import Snackbar from '@mui/material/Snackbar';
import type { AlertColor } from '@mui/material/Alert';

// SnackbarContext — RF-00-UX-DEFAULT-PATTERNS
// Toast global para feedback visual de sucesso/erro/aviso/info.
// Uso: const { showSnackbar } = useSnackbar();
//       showSnackbar('Item salvo com sucesso!', 'success');
//       showSnackbar('Erro ao salvar. Tente novamente.', 'error');

interface SnackbarState {
  open: boolean;
  message: string;
  severity: AlertColor;
  duration: number;
}

interface SnackbarContextValue {
  showSnackbar: (message: string, severity?: AlertColor, duration?: number) => void;
  showSuccess: (message: string) => void;
  showError: (message: string) => void;
  showWarning: (message: string) => void;
  showInfo: (message: string) => void;
}

const SnackbarContext = createContext<SnackbarContextValue>({
  showSnackbar: () => { },
  showSuccess: () => { },
  showError: () => { },
  showWarning: () => { },
  showInfo: () => { }
});

export function useSnackbar() {
  return useContext(SnackbarContext);
}

const DEFAULT_DURATION = 4000;
const ERROR_DURATION = 6000;

export function SnackbarProvider({ children }: { children: ReactNode }) {
  const [state, setState] = useState<SnackbarState>({
    open: false,
    message: '',
    severity: 'info',
    duration: DEFAULT_DURATION
  });

  const showSnackbar = useCallback(
    (message: string, severity: AlertColor = 'info', duration?: number) => {
      const d = duration ?? (severity === 'error' ? ERROR_DURATION : DEFAULT_DURATION);
      setState({ open: true, message, severity, duration: d });
    },
    []
  );

  const showSuccess = useCallback((msg: string) => showSnackbar(msg, 'success'), [showSnackbar]);
  const showError = useCallback((msg: string) => showSnackbar(msg, 'error'), [showSnackbar]);
  const showWarning = useCallback((msg: string) => showSnackbar(msg, 'warning'), [showSnackbar]);
  const showInfo = useCallback((msg: string) => showSnackbar(msg, 'info'), [showSnackbar]);

  const handleClose = (_: unknown, reason?: string) => {
    if (reason === 'clickaway') return;
    setState((s) => ({ ...s, open: false }));
  };

  return (
    <SnackbarContext.Provider value={{ showSnackbar, showSuccess, showError, showWarning, showInfo }}>
      {children}
      <Snackbar
        open={state.open}
        autoHideDuration={state.duration}
        onClose={handleClose}
        anchorOrigin={{ vertical: 'bottom', horizontal: 'center' }}
      >
        {/* Alert dentro de Snackbar para suportar severity e ação de fechar */}
        <Alert
          onClose={handleClose}
          severity={state.severity}
          variant="filled"
          sx={{ width: '100%', minWidth: 280, maxWidth: 480 }}
        >
          {state.message}
        </Alert>
      </Snackbar>
    </SnackbarContext.Provider>
  );
}
