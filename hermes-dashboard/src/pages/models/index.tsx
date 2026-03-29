import { useTranslation } from 'react-i18next';

import Grid from '@mui/material/Grid';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Chip from '@mui/material/Chip';

import CloudServerOutlined from '@ant-design/icons/CloudServerOutlined';

import EmptyState from 'components/EmptyState';
import SkeletonLoader from 'components/SkeletonLoader';
import { useModels } from 'api/gateway';

export default function ModelsPage() {
  const { t } = useTranslation();
  const { data: models, isLoading } = useModels();

  if (isLoading) return <SkeletonLoader variant="card" count={4} />;

  if (!models?.length) {
    return (
      <Stack spacing={3}>
        <Typography variant="h5">{t('models.title')}</Typography>
        <EmptyState title={t('models.empty')} icon={<CloudServerOutlined style={{ fontSize: 48 }} />} />
      </Stack>
    );
  }

  return (
    <Stack spacing={3}>
      <Typography variant="h5">{t('models.title')}</Typography>

      <Grid container spacing={2}>
        {models.map((model) => (
          <Grid size={{ xs: 12, sm: 6, md: 4 }} key={model.id}>
            <Card elevation={0} sx={{ border: '1px solid', borderColor: 'divider', height: '100%' }}>
              <CardContent>
                <Stack spacing={1.5}>
                  <Typography variant="h6" fontWeight={600}>{model.id}</Typography>
                  <Stack direction="row" spacing={1} alignItems="center">
                    <Typography variant="caption" color="text.secondary">{t('models.provider')}:</Typography>
                    <Chip label={model.owned_by} size="small" variant="outlined" />
                  </Stack>
                  {model.root && model.root !== model.id && (
                    <Stack direction="row" spacing={1} alignItems="center">
                      <Typography variant="caption" color="text.secondary">Root:</Typography>
                      <Chip label={model.root} size="small" color="primary" variant="outlined" />
                    </Stack>
                  )}
                </Stack>
              </CardContent>
            </Card>
          </Grid>
        ))}
      </Grid>
    </Stack>
  );
}
