import { useState } from 'react';
import { useTranslation } from 'react-i18next';

import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import Chip from '@mui/material/Chip';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import CircularProgress from '@mui/material/CircularProgress';

import SendOutlined from '@ant-design/icons/SendOutlined';

import MainCard from 'components/MainCard';
import EmptyState from 'components/EmptyState';
import SkeletonLoader from 'components/SkeletonLoader';
import { useWebhooks, testWebhook } from 'api/gateway';
import { useSnackbar } from 'contexts/SnackbarContext';

export default function WebhooksPage() {
  const { t } = useTranslation();
  const { data: webhooks, isLoading } = useWebhooks();
  const { showSuccess, showError } = useSnackbar();
  const [testing, setTesting] = useState(false);

  const handleTest = async () => {
    setTesting(true);
    try {
      await testWebhook();
      showSuccess(t('webhooks.testSuccess'));
    } catch {
      showError(t('webhooks.testFailed'));
    } finally {
      setTesting(false);
    }
  };

  if (isLoading) return <SkeletonLoader variant="table" rows={3} />;

  return (
    <Stack spacing={3}>
      <Stack direction="row" alignItems="center" justifyContent="space-between">
        <Typography variant="h5">{t('webhooks.title')}</Typography>
        <Button
          variant="outlined"
          startIcon={testing ? <CircularProgress size={16} /> : <SendOutlined />}
          onClick={handleTest}
          disabled={testing}
        >
          {t('webhooks.testWebhook')}
        </Button>
      </Stack>

      <MainCard contentSX={{ p: 0 }}>
        {!webhooks?.length ? (
          <EmptyState title={t('webhooks.empty')} />
        ) : (
          <TableContainer>
            <Table>
              <TableHead>
                <TableRow>
                  <TableCell>{t('webhooks.url')}</TableCell>
                  <TableCell>{t('webhooks.events')}</TableCell>
                  <TableCell>{t('common.status')}</TableCell>
                </TableRow>
              </TableHead>
              <TableBody>
                {webhooks.map((wh, idx) => (
                  <TableRow key={idx}>
                    <TableCell>
                      <Typography variant="body2" fontFamily="monospace">{wh.url}</Typography>
                    </TableCell>
                    <TableCell>
                      <Stack direction="row" gap={0.5} flexWrap="wrap">
                        {wh.events.map((ev) => (
                          <Chip key={ev} label={ev} size="small" variant="outlined" />
                        ))}
                      </Stack>
                    </TableCell>
                    <TableCell>
                      <Chip
                        label={wh.active ? t('common.active') : t('common.inactive')}
                        color={wh.active ? 'success' : 'default'}
                        size="small"
                      />
                    </TableCell>
                  </TableRow>
                ))}
              </TableBody>
            </Table>
          </TableContainer>
        )}
      </MainCard>
    </Stack>
  );
}
