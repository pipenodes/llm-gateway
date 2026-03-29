import { useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { useTranslation } from 'react-i18next';

import Box from '@mui/material/Box';
import Button from '@mui/material/Button';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';

import { APP_DEFAULT_PATH } from 'config';

// 404 — RF-00-UX-DEFAULT-ERRORS
export default function NotFound() {
  const navigate = useNavigate();
  const { t } = useTranslation();

  useEffect(() => {
    document.title = t('errors.notFound.docTitle');
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
        // spec: fade-in 200ms ease-out — RF-00-UX-DEFAULT-MOTION
        animation: 'fadeIn 0.2s ease-out',
        '@keyframes fadeIn': { from: { opacity: 0 }, to: { opacity: 1 } }
      }}
    >
      <Typography
        variant="h1"
        id="error-title"
        tabIndex={-1}
        sx={{ fontSize: { xs: '5rem', sm: '8rem' }, fontWeight: 700, color: 'text.disabled', lineHeight: 1 }}
        aria-label="Erro 404 — Página não encontrada"
      >
        404
      </Typography>

      <Stack spacing={1} alignItems="center">
        <Typography variant="h3">{t('errors.notFound.title')}</Typography>
        <Typography variant="body1" color="text.secondary" sx={{ maxWidth: 400 }}>
          {t('errors.notFound.description')}
        </Typography>
      </Stack>

      <Stack direction={{ xs: 'column', sm: 'row' }} spacing={2}>
        <Button
          variant="contained"
          size="large"
          onClick={() => navigate(APP_DEFAULT_PATH)}
          aria-label={t('errors.notFound.goHome')}
        >
          {t('errors.notFound.goHome')}
        </Button>
        <Button
          variant="outlined"
          size="large"
          onClick={() => navigate(-1)}
          aria-label={t('errors.notFound.goBack')}
        >
          {t('errors.notFound.goBack')}
        </Button>
      </Stack>
    </Box>
  );
}
