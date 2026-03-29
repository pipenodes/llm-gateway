import { useState } from 'react';

import Grid from '@mui/material/Grid';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import TextField from '@mui/material/TextField';
import Chip from '@mui/material/Chip';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';

import SecurityScanOutlined from '@ant-design/icons/SecurityScanOutlined';

import MainCard from 'components/MainCard';
import EmptyState from 'components/EmptyState';
import SkeletonLoader from 'components/SkeletonLoader';
import { useDLPQuarantine } from 'api/gateway';

const TAG_COLORS: Record<string, 'error' | 'warning' | 'info' | 'default'> = {
  pii: 'error',
  phi: 'warning',
  pci: 'warning',
  confidential: 'info'
};

function formatEpoch(epoch: number) {
  return new Date(epoch * 1000).toLocaleString();
}

export default function DLPPage() {
  const [tenant, setTenant] = useState('default');
  const { data: quarantine, isLoading } = useDLPQuarantine(tenant);

  const tagSummary = (quarantine ?? []).reduce<Record<string, number>>((acc, qe) => {
    acc[qe.tag] = (acc[qe.tag] ?? 0) + 1;
    return acc;
  }, {});

  return (
    <Stack spacing={3}>
      <Stack direction="row" alignItems="center" justifyContent="space-between" flexWrap="wrap" gap={2}>
        <Stack direction="row" alignItems="center" spacing={1}>
          <SecurityScanOutlined style={{ fontSize: 22 }} />
          <Typography variant="h5">Data Loss Prevention</Typography>
        </Stack>
        <TextField
          size="small"
          placeholder="Tenant ID"
          value={tenant}
          onChange={(e) => setTenant(e.target.value)}
          sx={{ width: 200 }}
        />
      </Stack>

      {/* Tag summary cards */}
      {Object.keys(tagSummary).length > 0 && (
        <Grid container spacing={2}>
          {Object.entries(tagSummary).map(([tag, count]) => (
            <Grid key={tag} size={{ xs: 6, sm: 4, md: 3 }}>
              <Card elevation={0} sx={{ border: '1px solid', borderColor: 'divider' }}>
                <CardContent>
                  <Stack direction="row" alignItems="center" justifyContent="space-between">
                    <Chip
                      label={tag.toUpperCase()}
                      size="small"
                      color={TAG_COLORS[tag] ?? 'default'}
                    />
                    <Typography variant="h4" fontWeight={700}>{count}</Typography>
                  </Stack>
                  <Typography variant="caption" color="text.secondary" sx={{ mt: 0.5, display: 'block' }}>
                    quarantined
                  </Typography>
                </CardContent>
              </Card>
            </Grid>
          ))}
        </Grid>
      )}

      <MainCard
        title="Quarantine"
        secondary={
          quarantine && quarantine.length > 0 ? (
            <Chip
              label={`${quarantine.length} item${quarantine.length !== 1 ? 's' : ''}`}
              color="error"
              size="small"
            />
          ) : undefined
        }
        contentSX={{ p: 0 }}
      >
        {isLoading ? (
          <SkeletonLoader variant="table" rows={5} />
        ) : !quarantine?.length ? (
          <EmptyState title="No quarantined content for this tenant" />
        ) : (
          <TableContainer>
            <Table size="small">
              <TableHead>
                <TableRow>
                  <TableCell>Request ID</TableCell>
                  <TableCell>App</TableCell>
                  <TableCell>Client</TableCell>
                  <TableCell>Tag</TableCell>
                  <TableCell>Reason</TableCell>
                  <TableCell>Time</TableCell>
                </TableRow>
              </TableHead>
              <TableBody>
                {quarantine.map((qe, idx) => (
                  <TableRow key={idx}>
                    <TableCell>
                      <Typography variant="caption" fontFamily="monospace" color="text.secondary">
                        {qe.request_id}
                      </Typography>
                    </TableCell>
                    <TableCell>
                      <Typography variant="body2">{qe.app_id}</Typography>
                    </TableCell>
                    <TableCell>
                      <Typography variant="caption">{qe.client_id}</Typography>
                    </TableCell>
                    <TableCell>
                      <Chip
                        label={qe.tag.toUpperCase()}
                        size="small"
                        color={TAG_COLORS[qe.tag] ?? 'default'}
                      />
                    </TableCell>
                    <TableCell>
                      <Typography variant="caption" color="text.secondary">{qe.reason}</Typography>
                    </TableCell>
                    <TableCell>
                      <Typography variant="caption" color="text.secondary">
                        {formatEpoch(qe.timestamp)}
                      </Typography>
                    </TableCell>
                  </TableRow>
                ))}
              </TableBody>
            </Table>
          </TableContainer>
        )}
      </MainCard>

      <MainCard title="DLP Policy Reference">
        <Stack spacing={1}>
          {[
            { tag: 'pii', action: 'redact', desc: 'PII content is redacted inline — request continues with placeholders' },
            { tag: 'phi', action: 'block', desc: 'PHI content (HIPAA) is blocked outright — request returns 403' },
            { tag: 'pci', action: 'quarantine', desc: 'PCI content is quarantined and blocked — logged for compliance review' },
            { tag: 'confidential', action: 'alert', desc: 'Confidential content is flagged and logged — request continues' }
          ].map((row) => (
            <Stack key={row.tag} direction="row" alignItems="center" spacing={2} sx={{ py: 0.5 }}>
              <Chip label={row.tag.toUpperCase()} size="small" color={TAG_COLORS[row.tag] ?? 'default'} sx={{ minWidth: 90 }} />
              <Chip label={row.action} size="small" variant="outlined" sx={{ minWidth: 80 }} />
              <Typography variant="caption" color="text.secondary">{row.desc}</Typography>
            </Stack>
          ))}
        </Stack>
      </MainCard>
    </Stack>
  );
}
