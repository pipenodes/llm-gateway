import { useState } from 'react';
import { useTranslation } from 'react-i18next';
import useSWR from 'swr';

import Grid from '@mui/material/Grid';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Chip from '@mui/material/Chip';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableRow from '@mui/material/TableRow';
import Button from '@mui/material/Button';
import Dialog from '@mui/material/Dialog';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import Alert from '@mui/material/Alert';
import CircularProgress from '@mui/material/CircularProgress';
import Tooltip from '@mui/material/Tooltip';
import LockIcon from '@mui/icons-material/Lock';
import ExtensionIcon from '@mui/icons-material/Extension';
import DownloadIcon from '@mui/icons-material/Download';

import EmptyState from 'components/EmptyState';
import SkeletonLoader from 'components/SkeletonLoader';
import { usePlugins } from 'api/gateway';
import { marketplaceApi, type MarketplacePlugin } from 'lib/marketplaceApi';

function MarketplaceModal({ open, onClose }: { open: boolean; onClose: () => void }) {
  const { t } = useTranslation();
  const [tier, setTier] = useState<string>('');
  const [installing, setInstalling] = useState<string | null>(null);
  const [error, setError] = useState('');
  const [success, setSuccess] = useState('');

  const { data, isLoading } = useSWR(
    open ? ['marketplace-plugins', tier] : null,
    () => marketplaceApi.plugins(tier || undefined)
  );

  const handleInstall = async (plugin: MarketplacePlugin) => {
    setInstalling(plugin.name);
    setError('');
    setSuccess('');
    try {
      await marketplaceApi.install(plugin.name, plugin.version);
      setSuccess(`${plugin.name} v${plugin.version} installed. Reload gateway to activate.`);
    } catch (e: unknown) {
      setError((e as Error).message);
    } finally {
      setInstalling(null);
    }
  };

  return (
    <Dialog open={open} onClose={onClose} maxWidth="md" fullWidth>
      <DialogTitle>
        <Stack direction="row" alignItems="center" spacing={1}>
          <ExtensionIcon color="primary" />
          <span>Plugin Marketplace — HERMES</span>
        </Stack>
      </DialogTitle>
      <DialogContent>
        <Stack direction="row" spacing={1} mb={2}>
          {['', 'core', 'enterprise'].map(t => (
            <Chip key={t} label={t || 'All'} onClick={() => setTier(t)}
              color={tier === t ? 'primary' : 'default'} variant={tier === t ? 'filled' : 'outlined'} />
          ))}
        </Stack>

        {error   && <Alert severity="error"   sx={{ mb: 2 }}>{error}</Alert>}
        {success && <Alert severity="success" sx={{ mb: 2 }}>{success}</Alert>}

        {isLoading ? (
          <Stack alignItems="center" py={4}><CircularProgress /></Stack>
        ) : (
          <Grid container spacing={2}>
            {(data?.plugins ?? []).map(plugin => (
              <Grid size={{ xs: 12, sm: 6 }} key={plugin.name}>
                <Card elevation={0} sx={{ border: '1px solid', borderColor: 'divider' }}>
                  <CardContent>
                    <Stack direction="row" justifyContent="space-between" alignItems="flex-start">
                      <Stack spacing={0.5}>
                        <Typography fontWeight={700}>{plugin.name}</Typography>
                        <Typography variant="caption" color="text.secondary">{plugin.description}</Typography>
                      </Stack>
                      <Stack spacing={0.5} alignItems="flex-end">
                        <Chip label={plugin.tier} size="small" color={plugin.tier === 'enterprise' ? 'secondary' : 'default'} />
                        <Typography variant="caption">v{plugin.version}</Typography>
                      </Stack>
                    </Stack>
                    <Stack mt={1.5}>
                      {!plugin.accessible ? (
                        <Tooltip title="Upgrade your plan to install enterprise plugins">
                          <span>
                            <Button size="small" variant="outlined" disabled startIcon={<LockIcon />} fullWidth>
                              Requires upgrade
                            </Button>
                          </span>
                        </Tooltip>
                      ) : (
                        <Button size="small" variant="contained" fullWidth
                          startIcon={installing === plugin.name ? <CircularProgress size={14} /> : <DownloadIcon />}
                          disabled={!!installing}
                          onClick={() => handleInstall(plugin)}>
                          {installing === plugin.name ? 'Installing...' : 'Install'}
                        </Button>
                      )}
                    </Stack>
                  </CardContent>
                </Card>
              </Grid>
            ))}
          </Grid>
        )}
      </DialogContent>
    </Dialog>
  );
}

export default function PluginsPage() {
  const { t } = useTranslation();
  const { data: plugins, isLoading } = usePlugins();
  const [marketplaceOpen, setMarketplaceOpen] = useState(false);

  if (isLoading) return <SkeletonLoader variant="card" count={4} />;

  if (!plugins?.length) {
    return (
      <Stack spacing={3}>
        <Stack direction="row" alignItems="center" justifyContent="space-between">
          <Typography variant="h5">{t('plugins.title')}</Typography>
          <Button variant="contained" size="small" startIcon={<ExtensionIcon />}
            onClick={() => setMarketplaceOpen(true)}>
            Browse Marketplace
          </Button>
        </Stack>
        <EmptyState title={t('plugins.empty')} />
        <MarketplaceModal open={marketplaceOpen} onClose={() => setMarketplaceOpen(false)} />
      </Stack>
    );
  }

  return (
    <Stack spacing={3}>
      <Stack direction="row" alignItems="center" justifyContent="space-between">
        <Stack direction="row" spacing={1} alignItems="center">
          <Typography variant="h5">{t('plugins.title')}</Typography>
          <Chip label={`${plugins.length} ${t('common.active').toLowerCase()}`} color="primary" size="small" />
        </Stack>
        <Button variant="contained" size="small" startIcon={<ExtensionIcon />}
          onClick={() => setMarketplaceOpen(true)}>
          Browse Marketplace
        </Button>
      </Stack>
      <MarketplaceModal open={marketplaceOpen} onClose={() => setMarketplaceOpen(false)} />

      <Grid container spacing={2}>
        {plugins.map((plugin, idx) => (
          <Grid size={{ xs: 12, sm: 6, md: 4 }} key={plugin.name}>
            <Card elevation={0} sx={{ border: '1px solid', borderColor: 'divider', height: '100%' }}>
              <CardContent>
                <Stack spacing={1.5}>
                  <Stack direction="row" alignItems="center" justifyContent="space-between">
                    <Typography variant="h6" fontWeight={600}>{plugin.name}</Typography>
                    <Chip label={`#${idx + 1}`} size="small" variant="outlined" />
                  </Stack>

                  <Stack direction="row" spacing={1} flexWrap="wrap">
                    <Chip label={`v${plugin.version}`} size="small" color="info" variant="outlined" />
                    <Chip label={plugin.type ?? 'builtin'} size="small" variant="outlined" />
                    <Chip
                      label={plugin.enabled !== false ? t('common.active') : t('common.inactive')}
                      size="small"
                      color={plugin.enabled !== false ? 'success' : 'default'}
                      variant="outlined"
                    />
                  </Stack>

                  {plugin.stats && Object.keys(plugin.stats).length > 0 && (
                    <Table size="small">
                      <TableBody>
                        {Object.entries(plugin.stats).map(([key, val]) => (
                          <TableRow key={key}>
                            <TableCell sx={{ pl: 0, borderBottom: 'none', py: 0.5 }}>
                              <Typography variant="caption" color="text.secondary">{key}</Typography>
                            </TableCell>
                            <TableCell align="right" sx={{ pr: 0, borderBottom: 'none', py: 0.5 }}>
                              <Typography variant="caption" fontWeight={600}>{String(val)}</Typography>
                            </TableCell>
                          </TableRow>
                        ))}
                      </TableBody>
                    </Table>
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

