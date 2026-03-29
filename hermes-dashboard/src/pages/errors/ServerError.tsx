import { Component, useEffect } from 'react';
import type { ReactNode, ErrorInfo } from 'react';
import { useNavigate, useRouteError } from 'react-router-dom';
import { useTranslation } from 'react-i18next';

import Box from '@mui/material/Box';
import Button from '@mui/material/Button';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';

import { APP_DEFAULT_PATH } from 'config';

// ── Error Boundary (class component obrigatório no React) ─────────────────────
interface ErrorBoundaryState {
  hasError: boolean;
  error: Error | null;
}

export class ErrorBoundary extends Component<{ children: ReactNode }, ErrorBoundaryState> {
  constructor(props: { children: ReactNode }) {
    super(props);
    this.state = { hasError: false, error: null };
  }

  static getDerivedStateFromError(error: Error): ErrorBoundaryState {
    return { hasError: true, error };
  }

  componentDidCatch(error: Error, info: ErrorInfo) {
    // Aqui integrar com Sentry/Datadog: captureException(error, { extra: info })
    console.error('[ErrorBoundary]', error, info);
  }

  render() {
    if (this.state.hasError) {
      return <ServerError traceId={undefined} />;
    }
    return this.props.children;
  }
}

// ── Página de erro 500 ────────────────────────────────────────────────────────
interface ServerErrorProps {
  traceId?: string;
}

export default function ServerError({ traceId }: ServerErrorProps) {
  const navigate = useNavigate();
  const { t } = useTranslation();

  useEffect(() => {
    document.title = t('errors.serverError.docTitle');
    document.getElementById('error-title')?.focus();
  }, [t]);

  return (
    <Box
      sx={{
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        justifyContent: 'center',
        minHeight: '60vh',
        textAlign: 'center',
        gap: 3,
        px: 3,
        animation: 'fadeIn 0.2s ease-out',
        '@keyframes fadeIn': { from: { opacity: 0 }, to: { opacity: 1 } }
      }}
    >
      <Typography
        variant="h1"
        id="error-title"
        tabIndex={-1}
        sx={{ fontSize: { xs: '5rem', sm: '8rem' }, fontWeight: 700, color: 'error.light', lineHeight: 1 }}
        aria-label="Erro 500 — Algo deu errado"
      >
        500
      </Typography>

      <Stack spacing={1} alignItems="center">
        <Typography variant="h3">{t('errors.serverError.title')}</Typography>
        <Typography variant="body1" color="text.secondary" sx={{ maxWidth: 420 }}>
          {t('errors.serverError.description')}
        </Typography>
        {traceId && (
          <Typography variant="caption" color="text.disabled">
            {t('errors.serverError.traceId', { id: traceId })}
          </Typography>
        )}
      </Stack>

      {/* spec: RF-00-UX-DEFAULT-ERRORS — 3 ações */}
      <Stack direction={{ xs: 'column', sm: 'row' }} spacing={2}>
        <Button
          variant="contained"
          size="large"
          onClick={() => window.location.reload()}
          aria-label={t('errors.serverError.retry')}
        >
          {t('errors.serverError.retry')}
        </Button>
        <Button
          variant="outlined"
          size="large"
          onClick={() => navigate(APP_DEFAULT_PATH)}
          aria-label={t('errors.serverError.goHome')}
        >
          {t('errors.serverError.goHome')}
        </Button>
        <Button
          variant="text"
          size="large"
          component="a"
          href="mailto:suporte@empresa.com"
          aria-label={t('errors.serverError.contactSupport')}
        >
          {t('errors.serverError.contactSupport')}
        </Button>
      </Stack>
    </Box>
  );
}

// ── Wrapper para uso como errorElement do React Router ────────────────────────
export function ServerErrorRoute() {
  const routerError = useRouteError();
  const traceId = routerError instanceof Error ? routerError.message : undefined;
  return <ServerError traceId={traceId} />;
}
