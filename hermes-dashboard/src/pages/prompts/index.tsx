import { useState } from 'react';
import { useTranslation } from 'react-i18next';
import { useFormik } from 'formik';
import * as Yup from 'yup';

import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import Dialog from '@mui/material/Dialog';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import DialogActions from '@mui/material/DialogActions';
import TextField from '@mui/material/TextField';
import Chip from '@mui/material/Chip';
import Tabs from '@mui/material/Tabs';
import Tab from '@mui/material/Tab';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import Box from '@mui/material/Box';

import PlusOutlined from '@ant-design/icons/PlusOutlined';

import MainCard from 'components/MainCard';
import EmptyState from 'components/EmptyState';
import SkeletonLoader from 'components/SkeletonLoader';
import { usePromptTemplates, useGuardrails } from 'api/gateway';
import { useSnackbar } from 'contexts/SnackbarContext';

const ACTION_COLORS: Record<string, 'error' | 'warning' | 'info'> = {
  block: 'error',
  warn: 'warning',
  redact: 'info'
};

export default function PromptsPage() {
  const { t } = useTranslation();
  const { data: templates, isLoading: templatesLoading, createTemplate } = usePromptTemplates();
  const { data: guardrails, isLoading: guardrailsLoading } = useGuardrails();
  const { showSuccess } = useSnackbar();
  const [tab, setTab] = useState(0);
  const [createOpen, setCreateOpen] = useState(false);

  const formik = useFormik({
    initialValues: { name: '', content: '', variables: '' },
    validationSchema: Yup.object({
      name: Yup.string().required(),
      content: Yup.string().required()
    }),
    onSubmit: async (values, { resetForm }) => {
      await createTemplate({
        name: values.name,
        content: values.content,
        variables: values.variables ? values.variables.split(',').map((s) => s.trim()) : []
      });
      showSuccess(t('common.create'));
      resetForm();
      setCreateOpen(false);
    }
  });

  const loading = templatesLoading || guardrailsLoading;
  if (loading) return <SkeletonLoader variant="table" rows={5} />;

  return (
    <Stack spacing={3}>
      <Stack direction="row" alignItems="center" justifyContent="space-between">
        <Typography variant="h5">{t('prompts.title')}</Typography>
        {tab === 0 && (
          <Button variant="contained" startIcon={<PlusOutlined />} onClick={() => setCreateOpen(true)}>
            {t('prompts.createTemplate')}
          </Button>
        )}
      </Stack>

      <Box sx={{ borderBottom: 1, borderColor: 'divider' }}>
        <Tabs value={tab} onChange={(_, v) => setTab(v)}>
          <Tab label={t('prompts.templates')} />
          <Tab label={t('prompts.guardrails')} />
        </Tabs>
      </Box>

      {tab === 0 && (
        <MainCard contentSX={{ p: 0 }}>
          {!templates?.length ? (
            <EmptyState
              title={t('prompts.emptyTemplates')}
              action={{ label: t('prompts.createTemplate'), onClick: () => setCreateOpen(true) }}
            />
          ) : (
            <TableContainer>
              <Table>
                <TableHead>
                  <TableRow>
                    <TableCell>{t('prompts.templateName')}</TableCell>
                    <TableCell>{t('prompts.templateContent')}</TableCell>
                    <TableCell>{t('prompts.templateVars')}</TableCell>
                  </TableRow>
                </TableHead>
                <TableBody>
                  {templates.map((tpl) => (
                    <TableRow key={tpl.name}>
                      <TableCell>
                        <Typography fontWeight={600}>{tpl.name}</Typography>
                      </TableCell>
                      <TableCell>
                        <Typography
                          variant="body2"
                          sx={{ maxWidth: 400, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}
                        >
                          {tpl.content}
                        </Typography>
                      </TableCell>
                      <TableCell>
                        <Stack direction="row" gap={0.5} flexWrap="wrap">
                          {tpl.variables.map((v) => (
                            <Chip key={v} label={v} size="small" variant="outlined" />
                          ))}
                        </Stack>
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
        <MainCard contentSX={{ p: 0 }}>
          {!guardrails?.length ? (
            <EmptyState title={t('prompts.emptyGuardrails')} />
          ) : (
            <TableContainer>
              <Table>
                <TableHead>
                  <TableRow>
                    <TableCell>{t('prompts.pattern')}</TableCell>
                    <TableCell>{t('prompts.action')}</TableCell>
                    <TableCell>{t('prompts.message')}</TableCell>
                  </TableRow>
                </TableHead>
                <TableBody>
                  {guardrails.map((gr, idx) => (
                    <TableRow key={idx}>
                      <TableCell>
                        <Typography variant="body2" fontFamily="monospace">{gr.pattern}</Typography>
                      </TableCell>
                      <TableCell>
                        <Chip label={gr.action} size="small" color={ACTION_COLORS[gr.action] ?? 'default'} />
                      </TableCell>
                      <TableCell>{gr.message}</TableCell>
                    </TableRow>
                  ))}
                </TableBody>
              </Table>
            </TableContainer>
          )}
        </MainCard>
      )}

      <Dialog open={createOpen} onClose={() => setCreateOpen(false)} maxWidth="sm" fullWidth>
        <form onSubmit={formik.handleSubmit}>
          <DialogTitle>{t('prompts.createTemplate')}</DialogTitle>
          <DialogContent>
            <Stack spacing={2} sx={{ mt: 1 }}>
              <TextField
                label={t('prompts.templateName')}
                fullWidth
                autoFocus
                {...formik.getFieldProps('name')}
                error={formik.touched.name && Boolean(formik.errors.name)}
              />
              <TextField
                label={t('prompts.templateContent')}
                fullWidth
                multiline
                rows={4}
                {...formik.getFieldProps('content')}
                error={formik.touched.content && Boolean(formik.errors.content)}
              />
              <TextField
                label={t('prompts.templateVars')}
                fullWidth
                placeholder="var1, var2, var3"
                {...formik.getFieldProps('variables')}
              />
            </Stack>
          </DialogContent>
          <DialogActions>
            <Button onClick={() => setCreateOpen(false)}>{t('common.cancel')}</Button>
            <Button type="submit" variant="contained" disabled={formik.isSubmitting}>{t('common.create')}</Button>
          </DialogActions>
        </form>
      </Dialog>
    </Stack>
  );
}
