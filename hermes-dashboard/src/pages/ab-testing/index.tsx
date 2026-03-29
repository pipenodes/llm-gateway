import { useState } from 'react';
import { useTranslation } from 'react-i18next';
import { useFormik, FieldArray, FormikProvider } from 'formik';
import * as Yup from 'yup';

import Grid from '@mui/material/Grid';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import Dialog from '@mui/material/Dialog';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import DialogActions from '@mui/material/DialogActions';
import TextField from '@mui/material/TextField';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Chip from '@mui/material/Chip';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import IconButton from '@mui/material/IconButton';
import Tooltip from '@mui/material/Tooltip';

import PlusOutlined from '@ant-design/icons/PlusOutlined';
import DeleteOutlined from '@ant-design/icons/DeleteOutlined';

import EmptyState from 'components/EmptyState';
import ConfirmDialog from 'components/ConfirmDialog';
import SkeletonLoader from 'components/SkeletonLoader';
import { useABExperiments } from 'api/gateway';
import { useSnackbar } from 'contexts/SnackbarContext';

export default function ABTestingPage() {
  const { t } = useTranslation();
  const { data: experiments, isLoading, createExperiment, deleteExperiment } = useABExperiments();
  const { showSuccess } = useSnackbar();
  const [createOpen, setCreateOpen] = useState(false);
  const [deleteTarget, setDeleteTarget] = useState<string | null>(null);
  const [isDeleting, setIsDeleting] = useState(false);
  const [expanded, setExpanded] = useState<string | null>(null);

  const formik = useFormik({
    initialValues: {
      name: '',
      trigger: '',
      variants: [{ name: '', model: '', weight: 50 }, { name: '', model: '', weight: 50 }]
    },
    validationSchema: Yup.object({
      name: Yup.string().required(),
      trigger: Yup.string().required(),
      variants: Yup.array().of(
        Yup.object({
          name: Yup.string().required(),
          model: Yup.string().required(),
          weight: Yup.number().min(0).max(100).required()
        })
      ).min(2)
    }),
    onSubmit: async (values, { resetForm }) => {
      await createExperiment(values);
      showSuccess(t('common.create'));
      resetForm();
      setCreateOpen(false);
    }
  });

  const handleDelete = async () => {
    if (!deleteTarget) return;
    setIsDeleting(true);
    try {
      await deleteExperiment(deleteTarget);
      showSuccess(t('common.delete'));
    } finally {
      setIsDeleting(false);
      setDeleteTarget(null);
    }
  };

  if (isLoading) return <SkeletonLoader variant="card" count={3} />;

  return (
    <Stack spacing={3}>
      <Stack direction="row" alignItems="center" justifyContent="space-between">
        <Typography variant="h5">{t('abTesting.title')}</Typography>
        <Button variant="contained" startIcon={<PlusOutlined />} onClick={() => setCreateOpen(true)}>
          {t('abTesting.createExperiment')}
        </Button>
      </Stack>

      {!experiments?.length ? (
        <EmptyState
          title={t('abTesting.empty')}
          action={{ label: t('abTesting.createExperiment'), onClick: () => setCreateOpen(true) }}
        />
      ) : (
        <Grid container spacing={2}>
          {experiments.map((exp) => (
            <Grid size={{ xs: 12 }} key={exp.name}>
              <Card elevation={0} sx={{ border: '1px solid', borderColor: 'divider' }}>
                <CardContent>
                  <Stack direction="row" alignItems="center" justifyContent="space-between">
                    <Stack direction="row" alignItems="center" spacing={2}>
                      <Typography variant="h6" fontWeight={600}>{exp.name}</Typography>
                      <Chip label={exp.trigger} size="small" variant="outlined" />
                      <Chip
                        label={`${exp.total_requests} ${t('abTesting.totalRequests').toLowerCase()}`}
                        size="small"
                        color="primary"
                        variant="outlined"
                      />
                    </Stack>
                    <Stack direction="row" spacing={1}>
                      <Button size="small" onClick={() => setExpanded(expanded === exp.name ? null : exp.name)}>
                        {t('abTesting.results')}
                      </Button>
                      <Tooltip title={t('common.delete')}>
                        <IconButton size="small" color="error" onClick={() => setDeleteTarget(exp.name)}>
                          <DeleteOutlined />
                        </IconButton>
                      </Tooltip>
                    </Stack>
                  </Stack>

                  {expanded === exp.name && exp.variants.length > 0 && (
                    <TableContainer sx={{ mt: 2 }}>
                      <Table size="small">
                        <TableHead>
                          <TableRow>
                            <TableCell>{t('abTesting.variantName')}</TableCell>
                            <TableCell>{t('abTesting.variantModel')}</TableCell>
                            <TableCell align="right">{t('abTesting.variantWeight')}</TableCell>
                            <TableCell align="right">{t('abTesting.totalRequests')}</TableCell>
                            <TableCell align="right">{t('abTesting.avgLatency')}</TableCell>
                            <TableCell align="right">P95</TableCell>
                            <TableCell align="right">{t('abTesting.errorRate')}</TableCell>
                            <TableCell align="right">{t('abTesting.avgTokens')}</TableCell>
                          </TableRow>
                        </TableHead>
                        <TableBody>
                          {exp.variants.map((v) => (
                            <TableRow key={v.name}>
                              <TableCell><Typography fontWeight={600}>{v.name}</Typography></TableCell>
                              <TableCell>{v.model}</TableCell>
                              <TableCell align="right">{v.weight}%</TableCell>
                              <TableCell align="right">{v.requests}</TableCell>
                              <TableCell align="right">{v.avg_latency_ms.toFixed(0)}ms</TableCell>
                              <TableCell align="right">{v.p95_latency_ms.toFixed(0)}ms</TableCell>
                              <TableCell align="right">
                                <Chip
                                  label={`${(v.error_rate * 100).toFixed(1)}%`}
                                  size="small"
                                  color={v.error_rate > 0.05 ? 'error' : 'success'}
                                  variant="outlined"
                                />
                              </TableCell>
                              <TableCell align="right">{v.avg_tokens.toFixed(0)}</TableCell>
                            </TableRow>
                          ))}
                        </TableBody>
                      </Table>
                    </TableContainer>
                  )}
                </CardContent>
              </Card>
            </Grid>
          ))}
        </Grid>
      )}

      <Dialog open={createOpen} onClose={() => setCreateOpen(false)} maxWidth="md" fullWidth>
        <FormikProvider value={formik}>
          <form onSubmit={formik.handleSubmit}>
            <DialogTitle>{t('abTesting.createExperiment')}</DialogTitle>
            <DialogContent>
              <Stack spacing={2} sx={{ mt: 1 }}>
                <TextField
                  label={t('abTesting.experimentName')}
                  fullWidth
                  autoFocus
                  {...formik.getFieldProps('name')}
                  error={formik.touched.name && Boolean(formik.errors.name)}
                />
                <TextField
                  label={t('abTesting.trigger')}
                  fullWidth
                  placeholder="*"
                  {...formik.getFieldProps('trigger')}
                  error={formik.touched.trigger && Boolean(formik.errors.trigger)}
                />
                <Typography variant="subtitle2">{t('abTesting.variants')}</Typography>
                <FieldArray name="variants">
                  {({ push, remove }) => (
                    <Stack spacing={1.5}>
                      {formik.values.variants.map((_, idx) => (
                        <Stack direction="row" spacing={1} key={idx} alignItems="center">
                          <TextField
                            label={t('abTesting.variantName')}
                            size="small"
                            {...formik.getFieldProps(`variants.${idx}.name`)}
                          />
                          <TextField
                            label={t('abTesting.variantModel')}
                            size="small"
                            {...formik.getFieldProps(`variants.${idx}.model`)}
                          />
                          <TextField
                            label={t('abTesting.variantWeight')}
                            type="number"
                            size="small"
                            sx={{ width: 100 }}
                            {...formik.getFieldProps(`variants.${idx}.weight`)}
                          />
                          {formik.values.variants.length > 2 && (
                            <IconButton size="small" color="error" onClick={() => remove(idx)}>
                              <DeleteOutlined />
                            </IconButton>
                          )}
                        </Stack>
                      ))}
                      <Button
                        size="small"
                        startIcon={<PlusOutlined />}
                        onClick={() => push({ name: '', model: '', weight: 0 })}
                      >
                        {t('abTesting.variants')}
                      </Button>
                    </Stack>
                  )}
                </FieldArray>
              </Stack>
            </DialogContent>
            <DialogActions>
              <Button onClick={() => setCreateOpen(false)}>{t('common.cancel')}</Button>
              <Button type="submit" variant="contained" disabled={formik.isSubmitting}>{t('common.create')}</Button>
            </DialogActions>
          </form>
        </FormikProvider>
      </Dialog>

      <ConfirmDialog
        open={!!deleteTarget}
        title={t('abTesting.deleteTitle')}
        description={t('abTesting.deleteConfirm', { name: deleteTarget })}
        confirmLabel={t('common.delete')}
        confirmColor="error"
        loading={isDeleting}
        onConfirm={handleDelete}
        onCancel={() => setDeleteTarget(null)}
      />
    </Stack>
  );
}
