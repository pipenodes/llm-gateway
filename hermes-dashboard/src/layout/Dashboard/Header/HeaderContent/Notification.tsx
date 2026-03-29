import { useRef, useState } from 'react';
import { useTranslation } from 'react-i18next';

import useMediaQuery from '@mui/material/useMediaQuery';
import Badge from '@mui/material/Badge';
import ClickAwayListener from '@mui/material/ClickAwayListener';
import List from '@mui/material/List';
import ListItemButton from '@mui/material/ListItemButton';
import ListItemText from '@mui/material/ListItemText';
import Paper from '@mui/material/Paper';
import Popper from '@mui/material/Popper';
import Tooltip from '@mui/material/Tooltip';
import Typography from '@mui/material/Typography';
import Box from '@mui/material/Box';

import MainCard from 'components/MainCard';
import IconButton from 'components/@extended/IconButton';
import Transitions from 'components/@extended/Transitions';

import BellOutlined from '@ant-design/icons/BellOutlined';
import CheckCircleOutlined from '@ant-design/icons/CheckCircleOutlined';
import ReloadOutlined from '@ant-design/icons/ReloadOutlined';

// Componente recebe notificações via props — sem dados hardcoded
interface NotificationItem {
  id: string;
  message: string;
  timestamp: string;
  read?: boolean;
}

interface NotificationProps {
  items?: NotificationItem[];
  unreadCount?: number;
  onMarkAllRead?: () => void;
  // spec: RF-00-UX-DEFAULT-APP-SHELL — estado de erro com retry
  error?: boolean;
  onRetry?: () => void;
}

export default function Notification({ items = [], unreadCount = 0, onMarkAllRead, error, onRetry }: NotificationProps) {
  const { t } = useTranslation();
  const downMD = useMediaQuery((theme: import('@mui/material/styles').Theme) => theme.breakpoints.down('md'));

  const anchorRef = useRef<HTMLButtonElement>(null);
  const [open, setOpen] = useState(false);

  const handleToggle = () => setOpen((prev) => !prev);
  const handleClose = (event: MouseEvent | TouchEvent) => {
    if (anchorRef.current?.contains(event.target as Node)) return;
    setOpen(false);
  };

  return (
    <Box sx={{ flexShrink: 0, ml: 0.75 }}>
      <IconButton
        color="secondary"
        variant="light"
        sx={{ color: 'text.primary', bgcolor: open ? 'grey.100' : 'transparent' }}
        aria-label={t('shell.notifications')}
        ref={anchorRef}
        aria-controls={open ? 'notification-grow' : undefined}
        aria-haspopup="true"
        onClick={handleToggle}
      >
        <Badge badgeContent={unreadCount} color="primary">
          <BellOutlined />
        </Badge>
      </IconButton>

      <Popper
        placement={downMD ? 'bottom' : 'bottom-end'}
        open={open}
        anchorEl={anchorRef.current}
        role={undefined}
        transition
        disablePortal
        popperOptions={{ modifiers: [{ name: 'offset', options: { offset: [downMD ? -5 : 0, 9] } }] }}
      >
        {({ TransitionProps }) => (
          <Transitions type="grow" position={downMD ? 'top' : 'top-right'} in={open} {...TransitionProps}>
            <Paper sx={{ boxShadow: 4, width: '100%', minWidth: 285, maxWidth: { xs: 285, md: 380 } }}>
              <ClickAwayListener onClickAway={handleClose}>
                <MainCard
                  title={t('notifications.title')}
                  elevation={0}
                  border={false}
                  content={false}
                  secondary={
                    unreadCount > 0 ? (
                      <Tooltip title={t('shell.markAllRead')}>
                        <IconButton color="success" size="small" onClick={onMarkAllRead}>
                          <CheckCircleOutlined style={{ fontSize: '1.15rem' }} />
                        </IconButton>
                      </Tooltip>
                    ) : undefined
                  }
                >
                  <List component="nav" sx={{ p: 0 }}>
                    {error ? (
                      // spec: RF-00-UX-DEFAULT-APP-SHELL — estado de erro com retry
                      <ListItemButton onClick={onRetry} sx={{ py: 2, justifyContent: 'center', gap: 1 }}>
                        <ReloadOutlined style={{ fontSize: '1rem' }} />
                        <ListItemText
                          primary={
                            <Typography variant="body2" color="text.secondary" align="center">
                              {t('notifications.error')}
                            </Typography>
                          }
                        />
                      </ListItemButton>
                    ) : items.length === 0 ? (
                      <ListItemButton disabled sx={{ py: 2, justifyContent: 'center' }}>
                        <ListItemText
                          primary={
                            <Typography variant="body2" color="text.secondary" align="center">
                              {t('notifications.empty')}
                            </Typography>
                          }
                        />
                      </ListItemButton>
                    ) : (
                      items.map((n) => (
                        <ListItemButton key={n.id} divider selected={!n.read}>
                          <ListItemText
                            primary={<Typography variant="h6">{n.message}</Typography>}
                            secondary={n.timestamp}
                          />
                        </ListItemButton>
                      ))
                    )}
                  </List>
                </MainCard>
              </ClickAwayListener>
            </Paper>
          </Transitions>
        )}
      </Popper>
    </Box>
  );
}
