import { useEffect, useRef } from 'react';
import { useTranslation } from 'react-i18next';

import Box from '@mui/material/Box';
import Button from '@mui/material/Button';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';

// Maintenance — RF-00-UX-DEFAULT-ERRORS
// Página completamente estática — sem App Shell, sem dependência de API.
// Substituir STATUS_URL e returnEstimate pelo valor real do produto.
const STATUS_URL = 'https://status.empresa.com';

interface MaintenanceProps {
  returnEstimate?: string; // ex: "14/02/2026 às 03:00" — omitir se incerto
}

export default function Maintenance({ returnEstimate }: MaintenanceProps) {
  const headingRef = useRef<HTMLHeadingElement>(null);
  const { t } = useTranslation();

  useEffect(() => {
    document.title = t('errors.maintenance.docTitle');
    headingRef.current?.focus();
  }, [t]);

  return (
    <Box
      sx={{
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        justifyContent: 'center',
        minHeight: '100vh',
        textAlign: 'center',
        gap: 3,
        px: 3,
        bgcolor: 'background.default'
      }}
    >
      <Stack spacing={1} alignItems="center" sx={{ maxWidth: 480 }}>
        <Typography
          variant="h3"
          id="maintenance-title"
          tabIndex={-1}
          component="h1"
          ref={headingRef}
        >
          {t('errors.maintenance.title')}
        </Typography>
        <Typography variant="body1" color="text.secondary">
          {t('errors.maintenance.description')}
        </Typography>
        {returnEstimate && (
          <Typography variant="body2" color="text.secondary">
            {t('errors.maintenance.returnEstimate', { time: returnEstimate })}
          </Typography>
        )}
      </Stack>

      <Button
        variant="outlined"
        size="large"
        component="a"
        href={STATUS_URL}
        target="_blank"
        rel="noopener noreferrer"
        aria-label={t('errors.maintenance.checkStatus')}
      >
        {t('errors.maintenance.checkStatus')}
      </Button>
    </Box>
  );
}
