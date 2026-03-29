import { useState } from 'react';
import { useTranslation } from 'react-i18next';
import { useFormik } from 'formik';
import * as Yup from 'yup';

import Grid from '@mui/material/Grid';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import LinearProgress from '@mui/material/LinearProgress';
import Button from '@mui/material/Button';
import Dialog from '@mui/material/Dialog';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import DialogActions from '@mui/material/DialogActions';
import TextField from '@mui/material/TextField';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import Box from '@mui/material/Box';

import { PieChart } from '@mui/x-charts/PieChart';

import MainCard from 'components/MainCard';
import EmptyState from 'components/EmptyState';
import SkeletonLoader from 'components/SkeletonLoader';
import { useCosts, useCostByKey } from 'api/gateway';

export default function CostsPage() {
  const { t } = useTranslation();
  const { data: allCosts, isLoading } = useCosts();
  const [selectedKey, setSelectedKey] = useState<string | null>(null);
  const { data: keyCost, setBudget } = useCostByKey(selectedKey);
  const [budgetOpen, setBudgetOpen] = useState(false);

  const formik = useFormik({
    initialValues: { budget_limit: keyCost?.budget_limit ?? 0 },
    enableReinitialize: true,
    validationSchema: Yup.object({ budget_limit: Yup.number().min(0).required() }),
    onSubmit: async (values) => {
      await setBudget({ budget_limit: values.budget_limit });
      setBudgetOpen(false);
    }
  });

  if (isLoading) return <SkeletonLoader variant="table" rows={5} />;

  if (!allCosts?.length) {
    return (
      <Stack spacing={3}>
        <Typography variant="h5">{t('costs.title')}</Typography>
        <EmptyState title={t('common.noResults')} />
      </Stack>
    );
  }

  return (
    <Stack spacing={3}>
      <Typography variant="h5">{t('costs.title')}</Typography>

      <MainCard title={t('costs.title')} contentSX={{ p: 0 }}>
        <TableContainer>
          <Table>
            <TableHead>
              <TableRow>
                <TableCell>{t('keys.alias')}</TableCell>
                <TableCell align="right">{t('costs.totalCost')}</TableCell>
                <TableCell align="right">{t('costs.budget')}</TableCell>
                <TableCell align="right">{t('costs.budgetUsed')}</TableCell>
                <TableCell>{t('common.actions')}</TableCell>
              </TableRow>
            </TableHead>
            <TableBody>
              {allCosts.map((c) => (
                <TableRow
                  key={c.alias}
                  hover
                  sx={{ cursor: 'pointer' }}
                  onClick={() => setSelectedKey(c.alias)}
                  selected={selectedKey === c.alias}
                >
                  <TableCell>{c.alias}</TableCell>
                  <TableCell align="right">${c.total_cost.toFixed(4)}</TableCell>
                  <TableCell align="right">
                    {c.budget_limit > 0 ? `$${c.budget_limit.toFixed(2)}` : t('costs.noBudget')}
                  </TableCell>
                  <TableCell align="right">
                    {c.budget_limit > 0 && (
                      <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                        <LinearProgress
                          variant="determinate"
                          value={Math.min(c.budget_used_pct, 100)}
                          color={c.budget_used_pct > 90 ? 'error' : c.budget_used_pct > 70 ? 'warning' : 'primary'}
                          sx={{ width: 80, height: 6, borderRadius: 3 }}
                        />
                        <Typography variant="caption">{c.budget_used_pct.toFixed(0)}%</Typography>
                      </Box>
                    )}
                  </TableCell>
                  <TableCell>
                    <Button size="small" onClick={(e) => { e.stopPropagation(); setSelectedKey(c.alias); setBudgetOpen(true); }}>
                      {t('costs.setBudget')}
                    </Button>
                  </TableCell>
                </TableRow>
              ))}
            </TableBody>
          </Table>
        </TableContainer>
      </MainCard>

      {keyCost && keyCost.by_model && Object.keys(keyCost.by_model).length > 0 && (
        <Grid container spacing={2}>
          <Grid size={{ xs: 12, md: 4 }}>
            <Card elevation={0} sx={{ border: '1px solid', borderColor: 'divider' }}>
              <CardContent>
                <Typography variant="body2" color="text.secondary">{t('costs.totalCost')}</Typography>
                <Typography variant="h3" fontWeight={600}>${keyCost.total_cost.toFixed(4)}</Typography>
              </CardContent>
            </Card>
          </Grid>
          <Grid size={{ xs: 12, md: 4 }}>
            <Card elevation={0} sx={{ border: '1px solid', borderColor: 'divider' }}>
              <CardContent>
                <Typography variant="body2" color="text.secondary">{t('costs.budgetRemaining')}</Typography>
                <Typography variant="h3" fontWeight={600}>
                  {keyCost.budget_limit > 0 ? `$${keyCost.budget_remaining.toFixed(2)}` : '-'}
                </Typography>
              </CardContent>
            </Card>
          </Grid>
          <Grid size={{ xs: 12, md: 4 }}>
            <MainCard title={t('costs.costBreakdown')}>
              <Box sx={{ width: '100%', height: 250 }}>
                <PieChart
                  series={[{
                    data: Object.entries(keyCost.by_model).map(([name, m], i) => ({
                      id: i,
                      value: m.cost,
                      label: name
                    })),
                    innerRadius: 30
                  }]}
                  height={230}
                />
              </Box>
            </MainCard>
          </Grid>
        </Grid>
      )}

      <Dialog open={budgetOpen} onClose={() => setBudgetOpen(false)} maxWidth="xs" fullWidth>
        <form onSubmit={formik.handleSubmit}>
          <DialogTitle>{t('costs.setBudget')}</DialogTitle>
          <DialogContent>
            <TextField
              label={t('costs.budgetLimit')}
              type="number"
              fullWidth
              autoFocus
              sx={{ mt: 1 }}
              {...formik.getFieldProps('budget_limit')}
              error={formik.touched.budget_limit && Boolean(formik.errors.budget_limit)}
            />
          </DialogContent>
          <DialogActions>
            <Button onClick={() => setBudgetOpen(false)}>{t('common.cancel')}</Button>
            <Button type="submit" variant="contained" disabled={formik.isSubmitting}>{t('common.save')}</Button>
          </DialogActions>
        </form>
      </Dialog>
    </Stack>
  );
}
