import { useEffect } from 'react';
import { useNavigate, useLocation } from 'react-router-dom';
import { useTranslation } from 'react-i18next';

import Button from '@mui/material/Button';
import Dialog from '@mui/material/Dialog';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import DialogContentText from '@mui/material/DialogContentText';
import DialogTitle from '@mui/material/DialogTitle';

// SessionExpired — RF-00-UX-DEFAULT-ERRORS
// Modal bloqueante: preserva contexto visual, redireciona com ?redirectTo=
export default function SessionExpired() {
  const navigate = useNavigate();
  const location = useLocation();
  const { t } = useTranslation();

  useEffect(() => {
    document.title = t('errors.sessionExpired.docTitle');
  }, [t]);

  const handleLogin = () => {
    const redirectTo = encodeURIComponent(location.pathname + location.search);
    navigate(`/login?redirectTo=${redirectTo}`, { replace: true });
  };

  return (
    <Dialog
      open
      maxWidth="xs"
      fullWidth
      disableEscapeKeyDown
      aria-labelledby="session-expired-title"
      aria-describedby="session-expired-description"
    >
      <DialogTitle id="session-expired-title">{t('errors.sessionExpired.title')}</DialogTitle>
      <DialogContent>
        <DialogContentText id="session-expired-description">
          {t('errors.sessionExpired.description')}
        </DialogContentText>
      </DialogContent>
      <DialogActions sx={{ px: 3, pb: 2 }}>
        <Button
          variant="contained"
          onClick={handleLogin}
          fullWidth
          size="large"
          autoFocus
          aria-label={t('errors.sessionExpired.loginAgain')}
        >
          {t('errors.sessionExpired.loginAgain')}
        </Button>
      </DialogActions>
    </Dialog>
  );
}
