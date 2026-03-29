import { useTranslation } from 'react-i18next';

import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';

// Footer genérico — personalize com informações do produto
export default function Footer() {
  const { t } = useTranslation();
  const year = new Date().getFullYear();

  return (
    <Stack
      direction={{ xs: 'column', sm: 'row' }}
      sx={{
        justifyContent: 'space-between',
        alignItems: { xs: 'center', sm: 'flex-end' },
        mt: 'auto',
        pt: 3,
        pb: 2
      }}
    >
      <Typography variant="caption" color="text.secondary">
        {t('footer.copyright', { year })}
      </Typography>
    </Stack>
  );
}
