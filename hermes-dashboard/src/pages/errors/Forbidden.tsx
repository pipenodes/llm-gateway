import { useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { useTranslation } from 'react-i18next';

import Box from '@mui/material/Box';
import Button from '@mui/material/Button';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';

import { APP_DEFAULT_PATH } from 'config';

// 403 — RF-00-UX-DEFAULT-ERRORS
// Não revelar se o recurso existe (segurança — evitar enumeração)
interface ForbiddenProps {
  // Configurável via prop para evitar email hardcoded — passar via env/config do produto
  supportEmail?: string;
}

export default function Forbidden({ supportEmail = 'suporte@empresa.com' }: ForbiddenProps) {
  const navigate = useNavigate();
  const { t } = useTranslation();

  useEffect(() => {
    document.title = t('errors.forbidden.docTitle');
    document.getElementById('error-title')?.focus();
    // Aqui registrar tentativa negada no audit log
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
        sx={{ fontSize: { xs: '5rem', sm: '8rem' }, fontWeight: 700, color: 'warning.light', lineHeight: 1 }}
        aria-label="Erro 403 — Acesso negado"
      >
        403
      </Typography>

      <Stack spacing={1} alignItems="center">
        <Typography variant="h3">{t('errors.forbidden.title')}</Typography>
        <Typography variant="body1" color="text.secondary" sx={{ maxWidth: 440 }}>
          {t('errors.forbidden.description')}
        </Typography>
        <Typography variant="body2" color="text.secondary">
          {t('errors.forbidden.hint')}
        </Typography>
      </Stack>

      <Stack direction={{ xs: 'column', sm: 'row' }} spacing={2}>
        <Button
          variant="contained"
          size="large"
          onClick={() => navigate(APP_DEFAULT_PATH)}
          aria-label={t('errors.forbidden.goHome')}
        >
          {t('errors.forbidden.goHome')}
        </Button>
        <Button
          variant="outlined"
          size="large"
          component="a"
          href={`mailto:${supportEmail}`}
          aria-label={t('errors.forbidden.contactSupport')}
        >
          {t('errors.forbidden.contactSupport')}
        </Button>
      </Stack>
    </Box>
  );
}
