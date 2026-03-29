import { useState } from 'react';
import { useTranslation } from 'react-i18next';

import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import IconButton from '@mui/material/IconButton';
import Tooltip from '@mui/material/Tooltip';
import Chip from '@mui/material/Chip';
import Dialog from '@mui/material/Dialog';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import DialogActions from '@mui/material/DialogActions';
import Button from '@mui/material/Button';
import Paper from '@mui/material/Paper';
import Box from '@mui/material/Box';

import DeleteOutlined from '@ant-design/icons/DeleteOutlined';
import EyeOutlined from '@ant-design/icons/EyeOutlined';

import { DataGrid } from '@mui/x-data-grid';
import type { GridColDef } from '@mui/x-data-grid';

import MainCard from 'components/MainCard';
import EmptyState from 'components/EmptyState';
import ConfirmDialog from 'components/ConfirmDialog';
import SkeletonLoader from 'components/SkeletonLoader';
import { useSessions, useSession, deleteSession } from 'api/gateway';
import { useSnackbar } from 'contexts/SnackbarContext';
import type { Session } from 'types/gateway';

export default function SessionsPage() {
  const { t } = useTranslation();
  const { data: sessions, isLoading, mutate } = useSessions();
  const { showSuccess } = useSnackbar();
  const [deleteTarget, setDeleteTarget] = useState<string | null>(null);
  const [viewTarget, setViewTarget] = useState<string | null>(null);
  const [isDeleting, setIsDeleting] = useState(false);

  const { data: sessionDetail } = useSession(viewTarget);

  const handleDelete = async () => {
    if (!deleteTarget) return;
    setIsDeleting(true);
    try {
      await deleteSession(deleteTarget);
      await mutate();
      showSuccess(t('common.delete'));
    } finally {
      setIsDeleting(false);
      setDeleteTarget(null);
    }
  };

  const columns: GridColDef<Session>[] = [
    { field: 'session_id', headerName: t('sessions.sessionId'), flex: 1, minWidth: 200 },
    { field: 'key_alias', headerName: t('audit.keyAlias'), width: 130 },
    { field: 'message_count', headerName: t('sessions.messageCount'), width: 120 },
    { field: 'created_at', headerName: t('keys.createdAt'), width: 180 },
    { field: 'last_activity', headerName: t('sessions.lastActivity'), width: 180 },
    {
      field: 'actions',
      headerName: t('common.actions'),
      width: 120,
      sortable: false,
      filterable: false,
      renderCell: (params) => (
        <Stack direction="row" spacing={0.5}>
          <Tooltip title={t('sessions.viewHistory')}>
            <IconButton size="small" onClick={() => setViewTarget(params.row.session_id)}>
              <EyeOutlined />
            </IconButton>
          </Tooltip>
          <Tooltip title={t('common.delete')}>
            <IconButton size="small" color="error" onClick={() => setDeleteTarget(params.row.session_id)}>
              <DeleteOutlined />
            </IconButton>
          </Tooltip>
        </Stack>
      )
    }
  ];

  if (isLoading) return <SkeletonLoader variant="table" rows={5} />;

  return (
    <Stack spacing={3}>
      <Typography variant="h5">{t('sessions.title')}</Typography>

      <MainCard contentSX={{ p: 0 }}>
        {!sessions?.length ? (
          <EmptyState title={t('sessions.empty')} />
        ) : (
          <DataGrid
            rows={sessions}
            columns={columns}
            getRowId={(row) => row.session_id}
            autoHeight
            disableRowSelectionOnClick
            pageSizeOptions={[10, 25, 50]}
            initialState={{ pagination: { paginationModel: { pageSize: 10 } } }}
          />
        )}
      </MainCard>

      <Dialog open={!!viewTarget} onClose={() => setViewTarget(null)} maxWidth="md" fullWidth>
        <DialogTitle>{t('sessions.conversation')}</DialogTitle>
        <DialogContent>
          {sessionDetail?.messages?.length ? (
            <Stack spacing={1.5} sx={{ mt: 1 }}>
              {sessionDetail.messages.map((msg, idx) => (
                <Paper
                  key={idx}
                  elevation={0}
                  sx={{
                    p: 2,
                    bgcolor: msg.role === 'user' ? 'primary.lighter' : msg.role === 'assistant' ? 'grey.50' : 'warning.lighter',
                    borderRadius: 2,
                    alignSelf: msg.role === 'user' ? 'flex-end' : 'flex-start',
                    maxWidth: '80%'
                  }}
                >
                  <Stack spacing={0.5}>
                    <Chip label={msg.role} size="small" variant="outlined" />
                    <Typography variant="body2" sx={{ whiteSpace: 'pre-wrap' }}>
                      {msg.content}
                    </Typography>
                    {msg.timestamp && (
                      <Typography variant="caption" color="text.secondary">{msg.timestamp}</Typography>
                    )}
                  </Stack>
                </Paper>
              ))}
            </Stack>
          ) : (
            <Box sx={{ py: 4, textAlign: 'center' }}>
              <Typography color="text.secondary">{t('common.noResults')}</Typography>
            </Box>
          )}
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setViewTarget(null)}>{t('common.close')}</Button>
        </DialogActions>
      </Dialog>

      <ConfirmDialog
        open={!!deleteTarget}
        title={t('sessions.deleteTitle')}
        description={t('sessions.deleteConfirm', { id: deleteTarget })}
        confirmLabel={t('common.delete')}
        confirmColor="error"
        loading={isDeleting}
        onConfirm={handleDelete}
        onCancel={() => setDeleteTarget(null)}
      />
    </Stack>
  );
}
