import Grid from '@mui/material/Grid';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Chip from '@mui/material/Chip';

import ShieldOutlined from '@ant-design/icons/SafetyOutlined';

import MainCard from 'components/MainCard';
import EmptyState from 'components/EmptyState';
import SkeletonLoader from 'components/SkeletonLoader';
import { useGuardrailsStats } from 'api/gateway';

function StatCard({ label, value, color }: { label: string; value: number; color?: string }) {
  return (
    <Card elevation={0} sx={{ border: '1px solid', borderColor: 'divider' }}>
      <CardContent>
        <Typography variant="body2" color="text.secondary">{label}</Typography>
        <Typography variant="h4" fontWeight={700} color={color ?? 'text.primary'} sx={{ mt: 0.5 }}>
          {value.toLocaleString()}
        </Typography>
      </CardContent>
    </Card>
  );
}

export default function GuardrailsPage() {
  const { data: stats, isLoading } = useGuardrailsStats();

  if (isLoading) return <SkeletonLoader variant="table" rows={3} />;

  if (!stats) {
    return (
      <Stack spacing={3}>
        <Stack direction="row" alignItems="center" spacing={1}>
          <ShieldOutlined style={{ fontSize: 22 }} />
          <Typography variant="h5">GuardRails</Typography>
        </Stack>
        <EmptyState title="GuardRails plugin not configured or not enabled" />
      </Stack>
    );
  }

  const blockRate = stats.total_requests > 0
    ? ((stats.l1_blocked / stats.total_requests) * 100).toFixed(1)
    : '0.0';

  return (
    <Stack spacing={3}>
      <Stack direction="row" alignItems="center" justifyContent="space-between">
        <Stack direction="row" alignItems="center" spacing={1}>
          <ShieldOutlined style={{ fontSize: 22 }} />
          <Typography variant="h5">GuardRails — LLM Firewall</Typography>
        </Stack>
        <Chip
          label={`Block rate: ${blockRate}%`}
          color={parseFloat(blockRate) > 5 ? 'error' : 'success'}
          size="small"
        />
      </Stack>

      <Grid container spacing={2}>
        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <StatCard label="Total Requests" value={stats.total_requests} />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <StatCard label="L1 Blocked" value={stats.l1_blocked} color="error.main" />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <StatCard label="L2 Blocked (ML)" value={stats.l2_blocked} color="warning.main" />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <StatCard label="Tenant Policies" value={stats.tenant_policies} />
        </Grid>
      </Grid>

      <Grid container spacing={2}>
        <Grid size={{ xs: 12, md: 6 }}>
          <MainCard title="L1 — Deterministic Security">
            <Stack spacing={1}>
              <Typography variant="body2">
                Synchronous layer — runs on every request before reaching the LLM backend.
              </Typography>
              <Stack spacing={0.5} sx={{ mt: 1 }}>
                {[
                  { label: 'Payload size check', desc: 'Blocks oversized requests (413)' },
                  { label: 'Model allow / deny list', desc: 'Enforces per-tenant model policy (403)' },
                  { label: 'Rate limit per tenant:client', desc: 'Token bucket, resets every minute (429)' },
                  { label: 'Prompt injection regex', desc: 'Weighted scoring — blocks at threshold (400)' }
                ].map((item) => (
                  <Stack key={item.label} direction="row" alignItems="flex-start" spacing={1} sx={{ py: 0.5 }}>
                    <Chip label="L1" size="small" color="primary" variant="outlined" sx={{ minWidth: 36 }} />
                    <Stack>
                      <Typography variant="body2" fontWeight={600}>{item.label}</Typography>
                      <Typography variant="caption" color="text.secondary">{item.desc}</Typography>
                    </Stack>
                  </Stack>
                ))}
              </Stack>
            </Stack>
          </MainCard>
        </Grid>

        <Grid size={{ xs: 12, md: 6 }}>
          <MainCard title="L2 / L3 — ML & Async Judge">
            <Stack spacing={1}>
              <Typography variant="body2">
                Optional layers enabled per-tenant. L2 uses ONNX classifiers; L3 is an
                asynchronous LLM Judge that never blocks the client response.
              </Typography>
              <Stack spacing={0.5} sx={{ mt: 1 }}>
                {[
                  { label: 'L2 — Toxicity / Jailbreak', desc: 'ONNX inference, configurable sample rate' },
                  { label: 'L3 — LLM Judge', desc: 'Async semantic analysis, fire-and-forget' }
                ].map((item) => (
                  <Stack key={item.label} direction="row" alignItems="flex-start" spacing={1} sx={{ py: 0.5 }}>
                    <Chip label={item.label.split(' ')[0]} size="small" color="warning" variant="outlined" sx={{ minWidth: 36 }} />
                    <Stack>
                      <Typography variant="body2" fontWeight={600}>{item.label}</Typography>
                      <Typography variant="caption" color="text.secondary">{item.desc}</Typography>
                    </Stack>
                  </Stack>
                ))}
              </Stack>
            </Stack>
          </MainCard>
        </Grid>
      </Grid>

      <MainCard title="Active Rate Buckets">
        <Typography variant="body2" color="text.secondary">
          Currently tracking{' '}
          <Typography component="span" fontWeight={700} color="text.primary">
            {stats.active_rate_buckets}
          </Typography>{' '}
          tenant:client combinations in the in-memory token bucket rate limiter.
          Buckets are created on first request and expire naturally as tokens refill.
        </Typography>
      </MainCard>
    </Stack>
  );
}
