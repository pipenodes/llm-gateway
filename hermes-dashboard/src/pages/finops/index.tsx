import { useState } from 'react';
import { useFormik } from 'formik';
import * as Yup from 'yup';

import Grid from '@mui/material/Grid';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import TextField from '@mui/material/TextField';
import Button from '@mui/material/Button';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import LinearProgress from '@mui/material/LinearProgress';
import Dialog from '@mui/material/Dialog';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import DialogActions from '@mui/material/DialogActions';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import Tabs from '@mui/material/Tabs';
import Tab from '@mui/material/Tab';

import DownloadOutlined from '@ant-design/icons/DownloadOutlined';
import FundOutlined from '@ant-design/icons/FundOutlined';
import EditOutlined from '@ant-design/icons/EditOutlined';

import MainCard from 'components/MainCard';
import EmptyState from 'components/EmptyState';
import SkeletonLoader from 'components/SkeletonLoader';
import { useFinOpsCosts, useFinOpsBudgets, downloadFinOpsExport } from 'api/gateway';
import { useSnackbar } from 'contexts/SnackbarContext';

function fmt(n: number) { return `$${n.toFixed(4)}`; }

export default function FinOpsPage() {
  const [tenant, setTenant]       = useState('default');
  const [tab, setTab]             = useState(0);
  const [budgetOpen, setBudgetOpen] = useState(false);
  const [exporting, setExporting] = useState(false);
  const { showSuccess, showError } = useSnackbar();

  const { data: costs,   isLoading: costsLoading }   = useFinOpsCosts(tenant);
  const { data: budgets, isLoading: budgetsLoading, setBudget } = useFinOpsBudgets(tenant);

  const loading = costsLoading || budgetsLoading;

  const tenantBudget = budgets?.[tenant];
  const usedPct = tenantBudget?.used_pct ?? 0;

  const formik = useFormik({
    initialValues: {
      monthly_limit_usd: tenantBudget?.monthly_limit_usd ?? 0,
      daily_limit_usd: tenantBudget?.daily_limit_usd ?? 0
    },
    enableReinitialize: true,
    validationSchema: Yup.object({
      monthly_limit_usd: Yup.number().min(0).required(),
      daily_limit_usd:   Yup.number().min(0).required()
    }),
    onSubmit: async (values) => {
      try {
        await setBudget(values);
        setBudgetOpen(false);
        showSuccess('Budget updated');
      } catch {
        showError('Failed to update budget');
      }
    }
  });

  const handleExport = async () => {
    setExporting(true);
    try {
      await downloadFinOpsExport();
      showSuccess('CSV exported');
    } catch {
      showError('Export failed');
    } finally {
      setExporting(false);
    }
  };

  const costEntries = Object.entries(costs ?? []);

  return (
    <Stack spacing={3}>
      <Stack direction="row" alignItems="center" justifyContent="space-between" flexWrap="wrap" gap={2}>
        <Stack direction="row" alignItems="center" spacing={1}>
          <FundOutlined style={{ fontSize: 22 }} />
          <Typography variant="h5">FinOps — Cost Governance</Typography>
        </Stack>
        <Stack direction="row" spacing={1}>
          <TextField
            size="small"
            placeholder="Tenant ID"
            value={tenant}
            onChange={(e) => setTenant(e.target.value)}
            sx={{ width: 180 }}
          />
          <Button
            variant="outlined"
            size="small"
            startIcon={<EditOutlined />}
            onClick={() => setBudgetOpen(true)}
          >
            Set Budget
          </Button>
          <Button
            variant="outlined"
            size="small"
            startIcon={<DownloadOutlined />}
            onClick={handleExport}
            disabled={exporting}
          >
            Export CSV
          </Button>
        </Stack>
      </Stack>

      {/* Budget summary */}
      {tenantBudget && (
        <Grid container spacing={2}>
          <Grid size={{ xs: 12, sm: 6, md: 3 }}>
            <Card elevation={0} sx={{ border: '1px solid', borderColor: 'divider' }}>
              <CardContent>
                <Typography variant="body2" color="text.secondary">Monthly Spent</Typography>
                <Typography variant="h4" fontWeight={700} sx={{ mt: 0.5 }}>
                  {fmt(tenantBudget.spent_monthly_usd)}
                </Typography>
                {tenantBudget.monthly_limit_usd > 0 && (
                  <Stack spacing={0.5} sx={{ mt: 1 }}>
                    <Typography variant="caption" color="text.secondary">
                      of {fmt(tenantBudget.monthly_limit_usd)} limit ({usedPct.toFixed(1)}%)
                    </Typography>
                    <LinearProgress
                      variant="determinate"
                      value={Math.min(usedPct, 100)}
                      color={usedPct >= 100 ? 'error' : usedPct >= 80 ? 'warning' : 'success'}
                      sx={{ height: 6, borderRadius: 3 }}
                    />
                  </Stack>
                )}
              </CardContent>
            </Card>
          </Grid>
          <Grid size={{ xs: 12, sm: 6, md: 3 }}>
            <Card elevation={0} sx={{ border: '1px solid', borderColor: 'divider' }}>
              <CardContent>
                <Typography variant="body2" color="text.secondary">Daily Spent</Typography>
                <Typography variant="h4" fontWeight={700} sx={{ mt: 0.5 }}>
                  {fmt(tenantBudget.spent_daily_usd)}
                </Typography>
                {tenantBudget.daily_limit_usd > 0 && (
                  <Typography variant="caption" color="text.secondary">
                    of {fmt(tenantBudget.daily_limit_usd)} daily limit
                  </Typography>
                )}
              </CardContent>
            </Card>
          </Grid>
        </Grid>
      )}

      <Tabs value={tab} onChange={(_, v) => setTab(v)} sx={{ borderBottom: 1, borderColor: 'divider' }}>
        <Tab label="Cost Breakdown" />
        <Tab label="Budget Hierarchy" />
      </Tabs>

      {tab === 0 && (
        <MainCard title="Cost Breakdown by Dimension" contentSX={{ p: 0 }}>
          {loading ? (
            <SkeletonLoader variant="table" rows={5} />
          ) : !costEntries.length ? (
            <EmptyState title="No cost data for this tenant yet" />
          ) : (
            <TableContainer>
              <Table size="small">
                <TableHead>
                  <TableRow>
                    <TableCell>Key (tenant → app → client)</TableCell>
                    <TableCell align="right">Requests</TableCell>
                    <TableCell align="right">Input Tokens</TableCell>
                    <TableCell align="right">Output Tokens</TableCell>
                    <TableCell align="right">Cost USD</TableCell>
                  </TableRow>
                </TableHead>
                <TableBody>
                  {costEntries
                    .sort((a, b) => b[1].cost_usd - a[1].cost_usd)
                    .map(([key, node]) => (
                      <TableRow key={key}>
                        <TableCell>
                          <Typography
                            variant="body2"
                            fontFamily="monospace"
                            fontWeight={key.split(':').length === 1 ? 700 : 400}
                          >
                            {key}
                          </Typography>
                        </TableCell>
                        <TableCell align="right">{node.requests.toLocaleString()}</TableCell>
                        <TableCell align="right">{node.input_tokens.toLocaleString()}</TableCell>
                        <TableCell align="right">{node.output_tokens.toLocaleString()}</TableCell>
                        <TableCell align="right">
                          <Typography
                            variant="body2"
                            fontWeight={600}
                            color={node.cost_usd > 1 ? 'error.main' : 'text.primary'}
                          >
                            {fmt(node.cost_usd)}
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
        <MainCard title="Budget Hierarchy" contentSX={{ p: 0 }}>
          {loading ? (
            <SkeletonLoader variant="table" rows={4} />
          ) : !Object.keys(budgets ?? {}).length ? (
            <EmptyState title="No budgets configured for this tenant" />
          ) : (
            <TableContainer>
              <Table size="small">
                <TableHead>
                  <TableRow>
                    <TableCell>Scope</TableCell>
                    <TableCell align="right">Monthly Limit</TableCell>
                    <TableCell align="right">Spent (Monthly)</TableCell>
                    <TableCell align="right">Daily Limit</TableCell>
                    <TableCell align="right">Spent (Daily)</TableCell>
                    <TableCell align="right">Usage %</TableCell>
                  </TableRow>
                </TableHead>
                <TableBody>
                  {Object.entries(budgets ?? {}).map(([key, b]) => {
                    const pct = b.used_pct ?? 0;
                    return (
                      <TableRow key={key}>
                        <TableCell>
                          <Typography variant="body2" fontFamily="monospace">{key}</Typography>
                        </TableCell>
                        <TableCell align="right">{b.monthly_limit_usd > 0 ? fmt(b.monthly_limit_usd) : '—'}</TableCell>
                        <TableCell align="right">{fmt(b.spent_monthly_usd)}</TableCell>
                        <TableCell align="right">{b.daily_limit_usd > 0 ? fmt(b.daily_limit_usd) : '—'}</TableCell>
                        <TableCell align="right">{fmt(b.spent_daily_usd)}</TableCell>
                        <TableCell align="right">
                          {b.monthly_limit_usd > 0 ? (
                            <Typography
                              variant="body2"
                              fontWeight={600}
                              color={pct >= 100 ? 'error.main' : pct >= 80 ? 'warning.main' : 'success.main'}
                            >
                              {pct.toFixed(1)}%
                            </Typography>
                          ) : '—'}
                        </TableCell>
                      </TableRow>
                    );
                  })}
                </TableBody>
              </Table>
            </TableContainer>
          )}
        </MainCard>
      )}

      {/* Budget set dialog */}
      <Dialog open={budgetOpen} onClose={() => setBudgetOpen(false)} maxWidth="xs" fullWidth>
        <form onSubmit={formik.handleSubmit}>
          <DialogTitle>Set Budget for "{tenant}"</DialogTitle>
          <DialogContent>
            <Stack spacing={2} sx={{ mt: 1 }}>
              <TextField
                fullWidth
                label="Monthly Limit (USD)"
                type="number"
                name="monthly_limit_usd"
                value={formik.values.monthly_limit_usd}
                onChange={formik.handleChange}
                error={!!formik.errors.monthly_limit_usd}
                helperText={formik.errors.monthly_limit_usd ?? 'Set to 0 to disable'}
                inputProps={{ min: 0, step: 0.01 }}
              />
              <TextField
                fullWidth
                label="Daily Limit (USD)"
                type="number"
                name="daily_limit_usd"
                value={formik.values.daily_limit_usd}
                onChange={formik.handleChange}
                error={!!formik.errors.daily_limit_usd}
                helperText={formik.errors.daily_limit_usd ?? 'Set to 0 to disable'}
                inputProps={{ min: 0, step: 0.01 }}
              />
            </Stack>
          </DialogContent>
          <DialogActions>
            <Button onClick={() => setBudgetOpen(false)}>Cancel</Button>
            <Button type="submit" variant="contained" disabled={formik.isSubmitting}>
              Save
            </Button>
          </DialogActions>
        </form>
      </Dialog>
    </Stack>
  );
}
