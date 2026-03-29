import { useState } from 'react';
import { useTranslation } from 'react-i18next';

import Grid from '@mui/material/Grid';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Chip from '@mui/material/Chip';
import LinearProgress from '@mui/material/LinearProgress';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import Tabs from '@mui/material/Tabs';
import Tab from '@mui/material/Tab';
import Box from '@mui/material/Box';
import CircularProgress from '@mui/material/CircularProgress';

import SafetyOutlined from '@ant-design/icons/SafetyOutlined';

import MainCard from 'components/MainCard';
import EmptyState from 'components/EmptyState';
import SkeletonLoader from 'components/SkeletonLoader';
import { useSecurityPosture, useComplianceReport, runSecurityScan } from 'api/gateway';
import { useSnackbar } from 'contexts/SnackbarContext';

const STATUS_COLORS: Record<string, 'success' | 'warning' | 'error'> = {
  pass: 'success',
  warn: 'warning',
  warning: 'warning',
  fail: 'error'
};

const COMPLIANCE_COLORS: Record<string, 'success' | 'warning' | 'error'> = {
  compliant: 'success',
  partial: 'warning',
  non_compliant: 'error'
};

export default function SecurityPage() {
  const { t } = useTranslation();
  const { data: posture, isLoading: postureLoading, mutate: mutatePosture } = useSecurityPosture();
  const { data: compliance, isLoading: complianceLoading } = useComplianceReport();
  const { showSuccess, showError } = useSnackbar();
  const [tab, setTab] = useState(0);
  const [scanning, setScanning] = useState(false);

  const handleScan = async () => {
    setScanning(true);
    try {
      await runSecurityScan();
      await mutatePosture();
      showSuccess(t('security.scan'));
    } catch {
      showError(t('common.error'));
    } finally {
      setScanning(false);
    }
  };

  const loading = postureLoading || complianceLoading;
  if (loading) return <SkeletonLoader variant="table" rows={5} />;

  return (
    <Stack spacing={3}>
      <Stack direction="row" alignItems="center" justifyContent="space-between">
        <Typography variant="h5">{t('security.title')}</Typography>
        <Button
          variant="outlined"
          startIcon={scanning ? <CircularProgress size={16} /> : <SafetyOutlined />}
          onClick={handleScan}
          disabled={scanning}
        >
          {t('security.runScan')}
        </Button>
      </Stack>

      <Box sx={{ borderBottom: 1, borderColor: 'divider' }}>
        <Tabs value={tab} onChange={(_, v) => setTab(v)}>
          <Tab label={t('security.posture')} />
          <Tab label={t('security.compliance')} />
        </Tabs>
      </Box>

      {tab === 0 && (
        <>
          {posture && (
            <Grid container spacing={2}>
              <Grid size={{ xs: 12, sm: 4 }}>
                <Card elevation={0} sx={{ border: '1px solid', borderColor: 'divider' }}>
                  <CardContent>
                    <Typography variant="body2" color="text.secondary">{t('security.score')}</Typography>
                    <Stack spacing={1} sx={{ mt: 1 }}>
                      <Typography variant="h3" fontWeight={700}>{posture.score}/100</Typography>
                      <LinearProgress
                        variant="determinate"
                        value={posture.score}
                        color={posture.score >= 80 ? 'success' : posture.score >= 50 ? 'warning' : 'error'}
                        sx={{ height: 8, borderRadius: 4 }}
                      />
                    </Stack>
                  </CardContent>
                </Card>
              </Grid>
            </Grid>
          )}

          <MainCard title={t('security.posture')} contentSX={{ p: 0 }}>
            {!posture?.checks?.length ? (
              <EmptyState title={t('security.empty')} />
            ) : (
              <TableContainer>
                <Table>
                  <TableHead>
                    <TableRow>
                      <TableCell>{t('common.name')}</TableCell>
                      <TableCell>{t('common.status')}</TableCell>
                      <TableCell>{t('common.details')}</TableCell>
                    </TableRow>
                  </TableHead>
                  <TableBody>
                    {posture.checks.map((check, idx) => (
                      <TableRow key={idx}>
                        <TableCell><Typography fontWeight={600}>{check.name}</Typography></TableCell>
                        <TableCell>
                          <Chip
                            label={t(`security.${check.status}`)}
                            color={STATUS_COLORS[check.status] ?? 'default'}
                            size="small"
                          />
                        </TableCell>
                        <TableCell>
                          <Typography variant="body2">{check.description}</Typography>
                          {check.recommendation && (
                            <Typography variant="caption" color="text.secondary">{check.recommendation}</Typography>
                          )}
                        </TableCell>
                      </TableRow>
                    ))}
                  </TableBody>
                </Table>
              </TableContainer>
            )}
          </MainCard>
        </>
      )}

      {tab === 1 && (
        <MainCard
          title={t('security.compliance')}
          secondary={
            compliance && (
              <Chip
                label={t(`security.${compliance.overall_status === 'non_compliant' ? 'nonCompliant' : compliance.overall_status}`)}
                color={COMPLIANCE_COLORS[compliance.overall_status] ?? 'default'}
                size="small"
              />
            )
          }
          contentSX={{ p: 0 }}
        >
          {!compliance?.checks?.length ? (
            <EmptyState title={t('security.empty')} />
          ) : (
            <TableContainer>
              <Table>
                <TableHead>
                  <TableRow>
                    <TableCell>{t('common.name')}</TableCell>
                    <TableCell>{t('common.type')}</TableCell>
                    <TableCell>{t('common.status')}</TableCell>
                    <TableCell>{t('common.details')}</TableCell>
                  </TableRow>
                </TableHead>
                <TableBody>
                  {compliance.checks.map((check, idx) => (
                    <TableRow key={idx}>
                      <TableCell><Typography fontWeight={600}>{check.name}</Typography></TableCell>
                      <TableCell><Chip label={check.category} size="small" variant="outlined" /></TableCell>
                      <TableCell>
                        <Chip
                          label={t(`security.${check.status === 'warning' ? 'warn' : check.status}`)}
                          color={STATUS_COLORS[check.status] ?? 'default'}
                          size="small"
                        />
                      </TableCell>
                      <TableCell>{check.details}</TableCell>
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
