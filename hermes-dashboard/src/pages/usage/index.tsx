import { useState } from 'react';
import { useTranslation } from 'react-i18next';

import Grid from '@mui/material/Grid';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Select from '@mui/material/Select';
import MenuItem from '@mui/material/MenuItem';
import FormControl from '@mui/material/FormControl';
import InputLabel from '@mui/material/InputLabel';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import Box from '@mui/material/Box';

import { BarChart } from '@mui/x-charts/BarChart';

import MainCard from 'components/MainCard';
import EmptyState from 'components/EmptyState';
import SkeletonLoader from 'components/SkeletonLoader';
import { useUsage, useUsageByKey } from 'api/gateway';

export default function UsagePage() {
  const { t } = useTranslation();
  const { data: allUsage, isLoading } = useUsage();
  const [selectedKey, setSelectedKey] = useState<string | null>(null);
  const { data: keyUsage } = useUsageByKey(selectedKey);

  if (isLoading) return <SkeletonLoader variant="table" rows={5} />;

  return (
    <Stack spacing={3}>
      <Stack direction="row" alignItems="center" justifyContent="space-between">
        <Typography variant="h5">{t('usage.title')}</Typography>
        <FormControl sx={{ minWidth: 200 }} size="small">
          <InputLabel>{t('audit.keyAlias')}</InputLabel>
          <Select
            value={selectedKey ?? ''}
            label={t('audit.keyAlias')}
            onChange={(e) => setSelectedKey(e.target.value || null)}
          >
            <MenuItem value="">{t('common.total')}</MenuItem>
            {allUsage?.map((u) => (
              <MenuItem key={u.alias} value={u.alias}>{u.alias}</MenuItem>
            ))}
          </Select>
        </FormControl>
      </Stack>

      {!allUsage?.length ? (
        <EmptyState title={t('common.noResults')} />
      ) : (
        <>
          {!selectedKey && (
            <MainCard title={t('usage.title')} contentSX={{ p: 0 }}>
              <TableContainer>
                <Table>
                  <TableHead>
                    <TableRow>
                      <TableCell>{t('keys.alias')}</TableCell>
                      <TableCell align="right">{t('usage.totalRequests')}</TableCell>
                      <TableCell align="right">{t('usage.totalTokens')}</TableCell>
                      <TableCell align="right">{t('usage.promptTokens')}</TableCell>
                      <TableCell align="right">{t('usage.completionTokens')}</TableCell>
                    </TableRow>
                  </TableHead>
                  <TableBody>
                    {allUsage.map((u) => (
                      <TableRow
                        key={u.alias}
                        hover
                        sx={{ cursor: 'pointer' }}
                        onClick={() => setSelectedKey(u.alias)}
                      >
                        <TableCell>{u.alias}</TableCell>
                        <TableCell align="right">{u.total_requests.toLocaleString()}</TableCell>
                        <TableCell align="right">{u.total_tokens.toLocaleString()}</TableCell>
                        <TableCell align="right">{u.total_prompt_tokens.toLocaleString()}</TableCell>
                        <TableCell align="right">{u.total_completion_tokens.toLocaleString()}</TableCell>
                      </TableRow>
                    ))}
                  </TableBody>
                </Table>
              </TableContainer>
            </MainCard>
          )}

          {keyUsage && (
            <Grid container spacing={2}>
              <Grid size={{ xs: 6, sm: 3 }}>
                <Card elevation={0} sx={{ border: '1px solid', borderColor: 'divider' }}>
                  <CardContent>
                    <Typography variant="body2" color="text.secondary">{t('usage.totalRequests')}</Typography>
                    <Typography variant="h4" fontWeight={600}>{keyUsage.total_requests.toLocaleString()}</Typography>
                  </CardContent>
                </Card>
              </Grid>
              <Grid size={{ xs: 6, sm: 3 }}>
                <Card elevation={0} sx={{ border: '1px solid', borderColor: 'divider' }}>
                  <CardContent>
                    <Typography variant="body2" color="text.secondary">{t('usage.totalTokens')}</Typography>
                    <Typography variant="h4" fontWeight={600}>{keyUsage.total_tokens.toLocaleString()}</Typography>
                  </CardContent>
                </Card>
              </Grid>
              <Grid size={{ xs: 6, sm: 3 }}>
                <Card elevation={0} sx={{ border: '1px solid', borderColor: 'divider' }}>
                  <CardContent>
                    <Typography variant="body2" color="text.secondary">{t('usage.promptTokens')}</Typography>
                    <Typography variant="h4" fontWeight={600}>{keyUsage.total_prompt_tokens.toLocaleString()}</Typography>
                  </CardContent>
                </Card>
              </Grid>
              <Grid size={{ xs: 6, sm: 3 }}>
                <Card elevation={0} sx={{ border: '1px solid', borderColor: 'divider' }}>
                  <CardContent>
                    <Typography variant="body2" color="text.secondary">{t('usage.completionTokens')}</Typography>
                    <Typography variant="h4" fontWeight={600}>{keyUsage.total_completion_tokens.toLocaleString()}</Typography>
                  </CardContent>
                </Card>
              </Grid>

              {keyUsage.by_model && Object.keys(keyUsage.by_model).length > 0 && (
                <Grid size={{ xs: 12, md: 6 }}>
                  <MainCard title={t('usage.byModel')}>
                    <Box sx={{ width: '100%', height: 300 }}>
                      <BarChart
                        xAxis={[{ scaleType: 'band', data: Object.keys(keyUsage.by_model) }]}
                        series={[
                          { data: Object.values(keyUsage.by_model).map((m) => m.requests), label: t('usage.totalRequests') },
                          { data: Object.values(keyUsage.by_model).map((m) => m.tokens), label: t('usage.totalTokens') }
                        ]}
                        height={280}
                      />
                    </Box>
                  </MainCard>
                </Grid>
              )}

              {keyUsage.daily && keyUsage.daily.length > 0 && (
                <Grid size={{ xs: 12, md: 6 }}>
                  <MainCard title={t('usage.daily')}>
                    <Box sx={{ width: '100%', height: 300 }}>
                      <BarChart
                        xAxis={[{ scaleType: 'band', data: keyUsage.daily.map((d) => d.date) }]}
                        series={[
                          { data: keyUsage.daily.map((d) => d.requests), label: t('usage.totalRequests') }
                        ]}
                        height={280}
                      />
                    </Box>
                  </MainCard>
                </Grid>
              )}
            </Grid>
          )}
        </>
      )}
    </Stack>
  );
}
