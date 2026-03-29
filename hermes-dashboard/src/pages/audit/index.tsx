import { useState } from 'react';
import { useTranslation } from 'react-i18next';

import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import Chip from '@mui/material/Chip';

import { DataGrid } from '@mui/x-data-grid';
import type { GridColDef, GridPaginationModel } from '@mui/x-data-grid';

import MainCard from 'components/MainCard';
import EmptyState from 'components/EmptyState';
import SkeletonLoader from 'components/SkeletonLoader';
import { useAudit } from 'api/gateway';

export default function AuditPage() {
  const { t } = useTranslation();
  const [paginationModel, setPaginationModel] = useState<GridPaginationModel>({
    pageSize: 25,
    page: 0
  });

  const offset = paginationModel.page * paginationModel.pageSize;
  const { data, isLoading } = useAudit(paginationModel.pageSize, offset);

  const columns: GridColDef[] = [
    { field: 'timestamp', headerName: t('audit.timestamp'), width: 180 },
    { field: 'key_alias', headerName: t('audit.keyAlias'), width: 130 },
    { field: 'method', headerName: t('audit.method'), width: 80 },
    { field: 'path', headerName: t('audit.path'), flex: 1, minWidth: 150 },
    { field: 'model', headerName: t('audit.model'), width: 180 },
    {
      field: 'status',
      headerName: t('audit.statusCode'),
      width: 100,
      renderCell: (params) => {
        const s = params.value as number;
        const color = s < 300 ? 'success' : s < 500 ? 'warning' : 'error';
        return <Chip label={s} color={color} size="small" variant="outlined" />;
      }
    },
    {
      field: 'latency_ms',
      headerName: t('audit.latency'),
      width: 120,
      valueFormatter: (value: number) => `${value?.toFixed(0) ?? '-'}ms`
    },
    {
      field: 'total_tokens',
      headerName: t('audit.tokens'),
      width: 100,
    },
    { field: 'request_id', headerName: t('audit.requestId'), width: 200 }
  ];

  if (isLoading) return <SkeletonLoader variant="table" rows={10} />;

  return (
    <Stack spacing={3}>
      <Typography variant="h5">{t('audit.title')}</Typography>

      <MainCard contentSX={{ p: 0 }}>
        {!data?.entries?.length ? (
          <EmptyState title={t('audit.empty')} />
        ) : (
          <DataGrid
            rows={data.entries}
            columns={columns}
            getRowId={(row) => `${row.timestamp}-${row.request_id}`}
            rowCount={data.total}
            paginationMode="server"
            paginationModel={paginationModel}
            onPaginationModelChange={setPaginationModel}
            pageSizeOptions={[10, 25, 50, 100]}
            autoHeight
            disableRowSelectionOnClick
          />
        )}
      </MainCard>
    </Stack>
  );
}
