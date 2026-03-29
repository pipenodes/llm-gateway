import { useState } from 'react';
import { useTranslation } from 'react-i18next';

import Grid from '@mui/material/Grid';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import Chip from '@mui/material/Chip';
import Box from '@mui/material/Box';

import MainCard from 'components/MainCard';
import ConfirmDialog from 'components/ConfirmDialog';
import SkeletonLoader from 'components/SkeletonLoader';
import { useGatewayConfig, useQueueStatus, useUpdateCheck } from 'api/gateway';
import { useSnackbar } from 'contexts/SnackbarContext';

export default function SettingsPage() {
  const { t } = useTranslation();
  const { data: config, isLoading: configLoading } = useGatewayConfig();
  const { data: queue, isLoading: queueLoading, clearQueue } = useQueueStatus();
  const { data: updates, isLoading: updatesLoading, mutate: recheckUpdates } = useUpdateCheck();
  const { showSuccess } = useSnackbar();
  const [clearQueueOpen, setClearQueueOpen] = useState(false);
  const [clearing, setClearing] = useState(false);

  const handleClearQueue = async () => {
    setClearing(true);
    try {
      await clearQueue();
      showSuccess(t('common.clear'));
    } finally {
      setClearing(false);
      setClearQueueOpen(false);
    }
  };

  const loading = configLoading || queueLoading || updatesLoading;
  if (loading) return <SkeletonLoader variant="card" count={3} />;

  return (
    <Stack spacing={3}>
      <Typography variant="h5">{t('settings.title')}</Typography>

      <Grid container spacing={2}>
        {queue && (
          <Grid size={{ xs: 12, md: 6 }}>
            <MainCard
              title={t('settings.queue')}
              secondary={
                <Button size="small" color="error" onClick={() => setClearQueueOpen(true)}>
                  {t('settings.clearQueue')}
                </Button>
              }
            >
              <Grid container spacing={2}>
                <Grid size={{ xs: 6 }}>
                  <Typography variant="body2" color="text.secondary">{t('settings.queuePending')}</Typography>
                  <Typography variant="h4" fontWeight={600}>{queue.metrics?.pending ?? 0}</Typography>
                </Grid>
                <Grid size={{ xs: 6 }}>
                  <Typography variant="body2" color="text.secondary">{t('settings.queueProcessing')}</Typography>
                  <Typography variant="h4" fontWeight={600}>{queue.active}</Typography>
                </Grid>
                <Grid size={{ xs: 6 }}>
                  <Typography variant="body2" color="text.secondary">{t('settings.queueMaxSize')}</Typography>
                  <Typography variant="h4" fontWeight={600}>{queue.max_size}</Typography>
                </Grid>
                <Grid size={{ xs: 6 }}>
                  <Typography variant="body2" color="text.secondary">{t('settings.queueWorkers')}</Typography>
                  <Typography variant="h4" fontWeight={600}>{queue.max_concurrency}</Typography>
                </Grid>
              </Grid>
            </MainCard>
          </Grid>
        )}

        {updates && (
          <Grid size={{ xs: 12, md: 6 }}>
            <MainCard
              title={t('settings.updates')}
              secondary={
                <Button size="small" onClick={() => recheckUpdates()}>
                  {t('settings.checkUpdates')}
                </Button>
              }
            >
              <Stack spacing={2}>
                <Stack direction="row" justifyContent="space-between" alignItems="center">
                  <Typography variant="body2" color="text.secondary">{t('settings.currentVersion')}</Typography>
                  <Chip label={updates.core.current} size="small" variant="outlined" />
                </Stack>
                {updates.core.latest && (
                  <Stack direction="row" justifyContent="space-between" alignItems="center">
                    <Typography variant="body2" color="text.secondary">{t('settings.latestVersion')}</Typography>
                    <Chip label={updates.core.latest} size="small" variant="outlined" />
                  </Stack>
                )}
                <Chip
                  label={updates.core.available ? t('settings.updateAvailable') : t('settings.upToDate')}
                  color={updates.core.available ? 'warning' : 'success'}
                  size="small"
                />
                {updates.message && (
                  <Typography variant="caption" color="text.secondary">{updates.message}</Typography>
                )}
              </Stack>
            </MainCard>
          </Grid>
        )}
      </Grid>

      {config && (
        <MainCard title={t('settings.gatewayConfig')}>
          <Box
            component="pre"
            sx={{
              p: 2,
              bgcolor: 'grey.50',
              borderRadius: 1,
              overflow: 'auto',
              maxHeight: 500,
              fontSize: '0.8125rem',
              fontFamily: 'monospace',
              m: 0
            }}
          >
            {JSON.stringify(config, null, 2)}
          </Box>
        </MainCard>
      )}

      <ConfirmDialog
        open={clearQueueOpen}
        title={t('settings.clearQueue')}
        description={t('settings.clearQueueConfirm')}
        confirmLabel={t('common.clear')}
        confirmColor="error"
        loading={clearing}
        onConfirm={handleClearQueue}
        onCancel={() => setClearQueueOpen(false)}
      />
    </Stack>
  );
}
