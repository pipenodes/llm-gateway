import { useTranslation } from 'react-i18next';

import Grid from '@mui/material/Grid';
import Typography from '@mui/material/Typography';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Stack from '@mui/material/Stack';
import Chip from '@mui/material/Chip';
import Skeleton from '@mui/material/Skeleton';
import Box from '@mui/material/Box';

import { BarChart } from '@mui/x-charts/BarChart';
import { PieChart } from '@mui/x-charts/PieChart';

import CheckCircleOutlined from '@ant-design/icons/CheckCircleOutlined';
import CloseCircleOutlined from '@ant-design/icons/CloseCircleOutlined';

import MainCard from 'components/MainCard';
import { useMetrics, useHealth } from 'api/gateway';

function formatUptime(seconds: number): string {
  const d = Math.floor(seconds / 86400);
  const h = Math.floor((seconds % 86400) / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  if (d > 0) return `${d}d ${h}h ${m}m`;
  if (h > 0) return `${h}h ${m}m`;
  return `${m}m`;
}

function formatBytes(kb: number): string {
  if (kb >= 1024 * 1024) return `${(kb / (1024 * 1024)).toFixed(1)} GB`;
  if (kb >= 1024) return `${(kb / 1024).toFixed(1)} MB`;
  return `${kb} KB`;
}

function formatNumber(n: number): string {
  if (n >= 1_000_000) return `${(n / 1_000_000).toFixed(1)}M`;
  if (n >= 1_000) return `${(n / 1_000).toFixed(1)}K`;
  return n.toLocaleString();
}

interface StatCardProps {
  title: string;
  value: string | number;
  loading?: boolean;
  color?: string;
}

function StatCard({ title, value, loading, color }: StatCardProps) {
  return (
    <Card elevation={0} sx={{ border: '1px solid', borderColor: 'divider', height: '100%' }}>
      <CardContent>
        <Typography variant="body2" color="text.secondary" gutterBottom>
          {title}
        </Typography>
        {loading ? (
          <Skeleton width={80} height={32} />
        ) : (
          <Typography variant="h4" fontWeight={600} sx={{ color }}>
            {value}
          </Typography>
        )}
      </CardContent>
    </Card>
  );
}

export default function DashboardPage() {
  const { t } = useTranslation();
  const { data: metrics, isLoading: metricsLoading } = useMetrics();
  const { data: health, isLoading: healthLoading } = useHealth();

  const isOnline = health?.status === 'ok';
  const cacheHits = metrics?.cache?.hits ?? 0;
  const cacheMisses = metrics?.cache?.misses ?? 0;
  const cacheTotal = cacheHits + cacheMisses;
  const cacheHitRate = cacheTotal > 0 ? ((cacheHits / cacheTotal) * 100).toFixed(1) : '0';

  const modelNames = metrics?.requests_per_model ? Object.keys(metrics.requests_per_model) : [];
  const requestsData = modelNames.map((m) => metrics?.requests_per_model?.[m] ?? 0);
  const tokensData = modelNames.map((m) => metrics?.tokens_per_model?.[m] ?? 0);
  const costData = modelNames.map((m) => metrics?.cost_per_model?.[m] ?? 0);

  const costPieData = modelNames.map((name, i) => ({
    id: i,
    value: costData[i],
    label: name
  }));

  return (
    <Stack spacing={3}>
      <Stack direction="row" alignItems="center" justifyContent="space-between">
        <Typography variant="h5">{t('dashboard.title')}</Typography>
        {!healthLoading && (
          <Chip
            icon={isOnline ? <CheckCircleOutlined /> : <CloseCircleOutlined />}
            label={isOnline ? t('dashboard.online') : t('dashboard.offline')}
            color={isOnline ? 'success' : 'error'}
            variant="outlined"
            size="small"
          />
        )}
      </Stack>

      <Grid container spacing={2}>
        <Grid size={{ xs: 6, sm: 4, md: 2 }}>
          <StatCard
            title={t('dashboard.uptime')}
            value={metrics ? formatUptime(metrics.uptime_seconds) : '-'}
            loading={metricsLoading}
          />
        </Grid>
        <Grid size={{ xs: 6, sm: 4, md: 2 }}>
          <StatCard
            title={t('dashboard.totalRequests')}
            value={metrics ? formatNumber(metrics.requests_total) : '-'}
            loading={metricsLoading}
          />
        </Grid>
        <Grid size={{ xs: 6, sm: 4, md: 2 }}>
          <StatCard
            title={t('dashboard.activeRequests')}
            value={metrics?.requests_active ?? '-'}
            loading={metricsLoading}
            color="primary.main"
          />
        </Grid>
        <Grid size={{ xs: 6, sm: 4, md: 2 }}>
          <StatCard
            title={t('dashboard.memoryUsage')}
            value={metrics ? formatBytes(metrics.memory_rss_kb) : '-'}
            loading={metricsLoading}
          />
        </Grid>
        <Grid size={{ xs: 6, sm: 4, md: 2 }}>
          <StatCard
            title={t('dashboard.cacheHitRate')}
            value={metrics ? `${cacheHitRate}%` : '-'}
            loading={metricsLoading}
          />
        </Grid>
        <Grid size={{ xs: 6, sm: 4, md: 2 }}>
          <StatCard
            title={t('dashboard.avgLatency')}
            value={metrics?.avg_latency_ms != null ? `${metrics.avg_latency_ms.toFixed(0)}ms` : '-'}
            loading={metricsLoading}
          />
        </Grid>
      </Grid>

      <Grid container spacing={2}>
        <Grid size={{ xs: 6, sm: 3 }}>
          <StatCard
            title={t('dashboard.totalTokens')}
            value={metrics?.total_tokens_used != null ? formatNumber(metrics.total_tokens_used) : '-'}
            loading={metricsLoading}
          />
        </Grid>
        <Grid size={{ xs: 6, sm: 3 }}>
          <StatCard
            title={t('dashboard.totalCost')}
            value={metrics?.total_cost != null ? `$${metrics.total_cost.toFixed(4)}` : '-'}
            loading={metricsLoading}
          />
        </Grid>
        <Grid size={{ xs: 6, sm: 3 }}>
          <StatCard
            title={t('dashboard.p95Latency')}
            value={metrics?.p95_latency_ms != null ? `${metrics.p95_latency_ms.toFixed(0)}ms` : '-'}
            loading={metricsLoading}
          />
        </Grid>
        <Grid size={{ xs: 6, sm: 3 }}>
          <StatCard
            title={t('dashboard.queuePending')}
            value={metrics?.plugins?.request_queue ? String((metrics.plugins.request_queue as Record<string, number>).pending ?? 0) : '-'}
            loading={metricsLoading}
          />
        </Grid>
      </Grid>

      {metrics && modelNames.length > 0 && (
        <Grid container spacing={2}>
          <Grid size={{ xs: 12, md: 4 }}>
            <MainCard title={t('dashboard.requestsByModel')}>
              <Box sx={{ width: '100%', height: 300 }}>
                <BarChart
                  xAxis={[{ scaleType: 'band', data: modelNames }]}
                  series={[{ data: requestsData, label: t('dashboard.requestsByModel') }]}
                  height={280}
                />
              </Box>
            </MainCard>
          </Grid>
          <Grid size={{ xs: 12, md: 4 }}>
            <MainCard title={t('dashboard.tokensByModel')}>
              <Box sx={{ width: '100%', height: 300 }}>
                <BarChart
                  xAxis={[{ scaleType: 'band', data: modelNames }]}
                  series={[{ data: tokensData, label: t('dashboard.tokensByModel') }]}
                  height={280}
                />
              </Box>
            </MainCard>
          </Grid>
          <Grid size={{ xs: 12, md: 4 }}>
            <MainCard title={t('dashboard.costByModel')}>
              <Box sx={{ width: '100%', height: 300 }}>
                <PieChart
                  series={[{ data: costPieData, innerRadius: 40 }]}
                  height={280}
                />
              </Box>
            </MainCard>
          </Grid>
        </Grid>
      )}
    </Stack>
  );
}
