import { useState } from 'react';
import { useTranslation } from 'react-i18next';
import { useFormik } from 'formik';
import * as Yup from 'yup';

import Button from '@mui/material/Button';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import Dialog from '@mui/material/Dialog';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import DialogActions from '@mui/material/DialogActions';
import TextField from '@mui/material/TextField';
import Alert from '@mui/material/Alert';
import IconButton from '@mui/material/IconButton';
import Tooltip from '@mui/material/Tooltip';

import PlusOutlined from '@ant-design/icons/PlusOutlined';
import DeleteOutlined from '@ant-design/icons/DeleteOutlined';
import CopyOutlined from '@ant-design/icons/CopyOutlined';

import { DataGrid } from '@mui/x-data-grid';
import type { GridColDef } from '@mui/x-data-grid';

import MainCard from 'components/MainCard';
import EmptyState from 'components/EmptyState';
import ConfirmDialog from 'components/ConfirmDialog';
import SkeletonLoader from 'components/SkeletonLoader';
import { useKeys } from 'api/gateway';
import { useSnackbar } from 'contexts/SnackbarContext';

const validationSchema = Yup.object({
  alias: Yup.string().required('keys.alias'),
  rate_limit_rpm: Yup.number().min(1).optional(),
  ip_whitelist: Yup.string().optional()
});

export default function KeysPage() {
  const { t } = useTranslation();
  const { data: keys, isLoading, createKey, deleteKey } = useKeys();
  const { showSuccess } = useSnackbar();
  const [createOpen, setCreateOpen] = useState(false);
  const [deleteAlias, setDeleteAlias] = useState<string | null>(null);
  const [createdKey, setCreatedKey] = useState<string | null>(null);
  const [isDeleting, setIsDeleting] = useState(false);

  const formik = useFormik({
    initialValues: { alias: '', rate_limit_rpm: 60, ip_whitelist: '' },
    validationSchema,
    onSubmit: async (values, { resetForm }) => {
      const res = await createKey({
        alias: values.alias,
        rate_limit_rpm: values.rate_limit_rpm,
        ip_whitelist: values.ip_whitelist ? values.ip_whitelist.split(',').map((s) => s.trim()) : []
      });
      setCreatedKey(res.key);
      showSuccess(t('keys.created'));
      resetForm();
      setCreateOpen(false);
    }
  });

  const handleDelete = async () => {
    if (!deleteAlias) return;
    setIsDeleting(true);
    try {
      await deleteKey(deleteAlias);
      showSuccess(t('common.delete'));
    } finally {
      setIsDeleting(false);
      setDeleteAlias(null);
    }
  };

  const handleCopyKey = async (key: string) => {
    await navigator.clipboard.writeText(key);
    showSuccess(t('common.copied'));
  };

  const columns: GridColDef[] = [
    { field: 'alias', headerName: t('keys.alias'), flex: 1, minWidth: 150 },
    { field: 'rate_limit_rpm', headerName: t('keys.rateLimit'), width: 150 },
    {
      field: 'ip_whitelist',
      headerName: t('keys.ipWhitelist'),
      flex: 1,
      minWidth: 200,
      valueFormatter: (value: string[]) => (value?.length ? value.join(', ') : '-')
    },
    { field: 'created_at', headerName: t('keys.createdAt'), width: 180 },
    {
      field: 'actions',
      headerName: t('common.actions'),
      width: 100,
      sortable: false,
      filterable: false,
      renderCell: (params) => (
        <Tooltip title={t('common.delete')}>
          <IconButton size="small" color="error" onClick={() => setDeleteAlias(params.row.alias)}>
            <DeleteOutlined />
          </IconButton>
        </Tooltip>
      )
    }
  ];

  if (isLoading) return <SkeletonLoader variant="table" rows={5} />;

  return (
    <Stack spacing={3}>
      <Stack direction="row" alignItems="center" justifyContent="space-between">
        <Typography variant="h5">{t('keys.title')}</Typography>
        <Button variant="contained" startIcon={<PlusOutlined />} onClick={() => setCreateOpen(true)}>
          {t('keys.createKey')}
        </Button>
      </Stack>

      {createdKey && (
        <Alert
          severity="success"
          action={
            <IconButton size="small" onClick={() => handleCopyKey(createdKey)}>
              <CopyOutlined />
            </IconButton>
          }
          onClose={() => setCreatedKey(null)}
        >
          <Typography variant="body2" fontWeight={600}>{t('keys.createdHint')}</Typography>
          <Typography variant="body2" fontFamily="monospace" sx={{ mt: 0.5 }}>
            {createdKey}
          </Typography>
        </Alert>
      )}

      <MainCard contentSX={{ p: 0 }}>
        {!keys?.length ? (
          <EmptyState
            title={t('keys.empty')}
            description={t('keys.emptyHint')}
            action={{ label: t('keys.createKey'), onClick: () => setCreateOpen(true) }}
          />
        ) : (
          <DataGrid
            rows={keys}
            columns={columns}
            getRowId={(row) => row.alias}
            autoHeight
            disableRowSelectionOnClick
            pageSizeOptions={[10, 25, 50]}
            initialState={{ pagination: { paginationModel: { pageSize: 10 } } }}
          />
        )}
      </MainCard>

      <Dialog open={createOpen} onClose={() => setCreateOpen(false)} maxWidth="sm" fullWidth>
        <form onSubmit={formik.handleSubmit}>
          <DialogTitle>{t('keys.createKey')}</DialogTitle>
          <DialogContent>
            <Stack spacing={2} sx={{ mt: 1 }}>
              <TextField
                label={t('keys.alias')}
                fullWidth
                autoFocus
                {...formik.getFieldProps('alias')}
                error={formik.touched.alias && Boolean(formik.errors.alias)}
                helperText={formik.touched.alias && formik.errors.alias}
              />
              <TextField
                label={t('keys.rateLimit')}
                type="number"
                fullWidth
                {...formik.getFieldProps('rate_limit_rpm')}
              />
              <TextField
                label={t('keys.ipWhitelist')}
                fullWidth
                placeholder="192.168.1.1, 10.0.0.0/8"
                {...formik.getFieldProps('ip_whitelist')}
              />
            </Stack>
          </DialogContent>
          <DialogActions>
            <Button onClick={() => setCreateOpen(false)}>{t('common.cancel')}</Button>
            <Button type="submit" variant="contained" disabled={formik.isSubmitting}>
              {t('common.create')}
            </Button>
          </DialogActions>
        </form>
      </Dialog>

      <ConfirmDialog
        open={!!deleteAlias}
        title={t('keys.deleteTitle')}
        description={t('keys.deleteConfirm', { alias: deleteAlias })}
        confirmLabel={t('common.delete')}
        confirmColor="error"
        loading={isDeleting}
        onConfirm={handleDelete}
        onCancel={() => setDeleteAlias(null)}
      />
    </Stack>
  );
}
