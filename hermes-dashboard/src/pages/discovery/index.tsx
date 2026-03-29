import { useState } from 'react';

import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import TextField from '@mui/material/TextField';
import Chip from '@mui/material/Chip';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import Tabs from '@mui/material/Tabs';
import Tab from '@mui/material/Tab';
import Box from '@mui/material/Box';
import InputAdornment from '@mui/material/InputAdornment';

import SearchOutlined from '@ant-design/icons/SearchOutlined';
import RadarChartOutlined from '@ant-design/icons/RadarChartOutlined';
import WarningOutlined from '@ant-design/icons/WarningOutlined';

import MainCard from 'components/MainCard';
import EmptyState from 'components/EmptyState';
import SkeletonLoader from 'components/SkeletonLoader';
import { useDiscoveryCatalog, useDiscoveryShadowAI } from 'api/gateway';

const TAG_COLORS: Record<string, 'error' | 'warning' | 'info' | 'default'> = {
  pii: 'error',
  phi: 'warning',
  pci: 'warning',
  confidential: 'info'
};

function formatEpoch(epoch: number) {
  return new Date(epoch * 1000).toLocaleString();
}

export default function DataDiscoveryPage() {
  const [tenant, setTenant]   = useState('default');
  const [search, setSearch]   = useState('');
  const [tab, setTab]         = useState(0);

  const { data: catalog,  isLoading: catLoading  } = useDiscoveryCatalog(tenant);
  const { data: shadowAI, isLoading: shadowLoading } = useDiscoveryShadowAI(tenant);

  const loading = catLoading || shadowLoading;

  const filteredCatalog = (catalog ?? []).filter((e) =>
    !search || e.tag.includes(search) || e.tenant_id.includes(search) || e.app_id.includes(search)
  );

  return (
    <Stack spacing={3}>
      <Stack direction="row" alignItems="center" justifyContent="space-between" flexWrap="wrap" gap={2}>
        <Stack direction="row" alignItems="center" spacing={1}>
          <RadarChartOutlined style={{ fontSize: 22 }} />
          <Typography variant="h5">Data Discovery</Typography>
        </Stack>
        <TextField
          size="small"
          placeholder="Tenant ID"
          value={tenant}
          onChange={(e) => setTenant(e.target.value)}
          sx={{ width: 200 }}
        />
      </Stack>

      <Box sx={{ borderBottom: 1, borderColor: 'divider' }}>
        <Tabs value={tab} onChange={(_, v) => setTab(v)}>
          <Tab label={`Content Catalog${catalog ? ` (${catalog.length})` : ''}`} />
          <Tab
            label={
              <Stack direction="row" alignItems="center" spacing={0.5}>
                <span>Shadow AI</span>
                {shadowAI && shadowAI.length > 0 && (
                  <Chip
                    label={shadowAI.length}
                    size="small"
                    color="error"
                    sx={{ height: 18, fontSize: 11 }}
                  />
                )}
              </Stack>
            }
          />
        </Tabs>
      </Box>

      {tab === 0 && (
        <MainCard
          title="Content Catalog"
          secondary={
            <TextField
              size="small"
              placeholder="Filter by tag..."
              value={search}
              onChange={(e) => setSearch(e.target.value)}
              InputProps={{
                startAdornment: (
                  <InputAdornment position="start">
                    <SearchOutlined style={{ fontSize: 14 }} />
                  </InputAdornment>
                )
              }}
              sx={{ width: 180 }}
            />
          }
          contentSX={{ p: 0 }}
        >
          {loading ? (
            <SkeletonLoader variant="table" rows={5} />
          ) : !filteredCatalog.length ? (
            <EmptyState title="No data patterns detected for this tenant yet" />
          ) : (
            <TableContainer>
              <Table size="small">
                <TableHead>
                  <TableRow>
                    <TableCell>Hash</TableCell>
                    <TableCell>Tag</TableCell>
                    <TableCell>App</TableCell>
                    <TableCell align="right">Occurrences</TableCell>
                    <TableCell>Last Seen</TableCell>
                  </TableRow>
                </TableHead>
                <TableBody>
                  {filteredCatalog.map((entry) => (
                    <TableRow key={entry.hash}>
                      <TableCell>
                        <Typography variant="caption" fontFamily="monospace" color="text.secondary">
                          {entry.hash.slice(0, 16)}…
                        </Typography>
                      </TableCell>
                      <TableCell>
                        <Chip
                          label={entry.tag.toUpperCase()}
                          size="small"
                          color={TAG_COLORS[entry.tag] ?? 'default'}
                        />
                      </TableCell>
                      <TableCell>
                        <Typography variant="body2">{entry.app_id}</Typography>
                      </TableCell>
                      <TableCell align="right">
                        <Typography variant="body2" fontWeight={600}>
                          {entry.occurrences.toLocaleString()}
                        </Typography>
                      </TableCell>
                      <TableCell>
                        <Typography variant="caption" color="text.secondary">
                          {formatEpoch(entry.last_seen)}
                        </Typography>
                      </TableCell>
                    </TableRow>
                  ))}
                </TableBody>
              </Table>
            </TableContainer>
          )}
        </MainCard>
      )}

      {tab === 1 && (
        <MainCard
          title="Shadow AI Detections"
          secondary={
            shadowAI && shadowAI.length > 0 ? (
              <Chip
                icon={<WarningOutlined />}
                label={`${shadowAI.length} uninventoried model${shadowAI.length !== 1 ? 's' : ''}`}
                color="error"
                size="small"
              />
            ) : undefined
          }
          contentSX={{ p: 0 }}
        >
          {loading ? (
            <SkeletonLoader variant="table" rows={3} />
          ) : !shadowAI?.length ? (
            <EmptyState title="No Shadow AI detected — all models are inventoried" />
          ) : (
            <TableContainer>
              <Table size="small">
                <TableHead>
                  <TableRow>
                    <TableCell>Model</TableCell>
                    <TableCell>App</TableCell>
                    <TableCell>Endpoint</TableCell>
                    <TableCell>Detected At</TableCell>
                  </TableRow>
                </TableHead>
                <TableBody>
                  {shadowAI.map((ev, idx) => (
                    <TableRow key={idx}>
                      <TableCell>
                        <Typography variant="body2" fontWeight={600} color="error.main">
                          {ev.model}
                        </Typography>
                      </TableCell>
                      <TableCell>
                        <Typography variant="body2">{ev.app_id}</Typography>
                      </TableCell>
                      <TableCell>
                        <Typography variant="caption" fontFamily="monospace">{ev.endpoint}</Typography>
                      </TableCell>
                      <TableCell>
                        <Typography variant="caption" color="text.secondary">
                          {formatEpoch(ev.detected_at)}
                        </Typography>
                      </TableCell>
                    </TableRow>
                  ))}
                </TableBody>
              </Table>
            </TableContainer>
          )}
        </MainCard>
      )}
    </Stack>
  );
}
