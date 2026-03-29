import { useRef, useState } from 'react';
import { useTranslation } from 'react-i18next';

import { useTheme } from '@mui/material/styles';
import ButtonBase from '@mui/material/ButtonBase';
import ClickAwayListener from '@mui/material/ClickAwayListener';
import Divider from '@mui/material/Divider';
import List from '@mui/material/List';
import ListItemButton from '@mui/material/ListItemButton';
import ListItemIcon from '@mui/material/ListItemIcon';
import ListItemText from '@mui/material/ListItemText';
import Paper from '@mui/material/Paper';
import Popper from '@mui/material/Popper';
import Stack from '@mui/material/Stack';
import Tooltip from '@mui/material/Tooltip';
import Typography from '@mui/material/Typography';
import Box from '@mui/material/Box';

import Avatar from 'components/@extended/Avatar';
import MainCard from 'components/MainCard';
import Transitions from 'components/@extended/Transitions';
import IconButton from 'components/@extended/IconButton';
import ThemeToggleMenuItem from './ThemeToggle';
import LanguageSwitcher from './LanguageSwitcher';

import LogoutOutlined from '@ant-design/icons/LogoutOutlined';
import UserOutlined from '@ant-design/icons/UserOutlined';
import SettingOutlined from '@ant-design/icons/SettingOutlined';
import QuestionCircleOutlined from '@ant-design/icons/QuestionCircleOutlined';
import SwapOutlined from '@ant-design/icons/SwapOutlined';

// User info é passado via props — sem dados hardcoded
interface ProfileProps {
  user?: {
    name?: string;
    role?: string;
    avatarUrl?: string;
  };
  onLogout?: () => void;
  onAccount?: () => void;
  onSwitchAccount?: () => void;
  onSettings?: () => void;
  onHelp?: () => void;
}

export default function Profile({
  user,
  onLogout,
  onAccount,
  onSwitchAccount,
  onSettings,
  onHelp
}: ProfileProps) {
  const { t } = useTranslation();
  const theme = useTheme();
  const anchorRef = useRef<HTMLButtonElement>(null);
  const [open, setOpen] = useState(false);

  const handleToggle = () => setOpen((prev) => !prev);
  const handleClose = (event: MouseEvent | TouchEvent) => {
    if (anchorRef.current?.contains(event.target as Node)) return;
    setOpen(false);
  };

  const closeMenu = () => setOpen(false);

  return (
    <Box sx={{ flexShrink: 0, ml: 'auto' }}>
      <Tooltip title={t('shell.openProfile')} disableInteractive>
        <ButtonBase
          sx={{
            p: 0.25,
            borderRadius: 1,
            '&:focus-visible': {
              outline: `2px solid ${theme.palette.secondary.dark}`,
              outlineOffset: 2
            }
          }}
          aria-label={t('shell.openProfile')}
          ref={anchorRef}
          aria-controls={open ? 'profile-grow' : undefined}
          aria-haspopup="true"
          onClick={handleToggle}
        >
          {user?.avatarUrl ? (
            <Avatar alt={user.name ?? 'Usuário'} src={user.avatarUrl} size="sm"
              sx={{ '&:hover': { outline: '1px solid', outlineColor: 'primary.main' } }}
            />
          ) : (
            <Avatar size="sm" sx={{ '&:hover': { outline: '1px solid', outlineColor: 'primary.main' } }}>
              <UserOutlined />
            </Avatar>
          )}
        </ButtonBase>
      </Tooltip>

      <Popper
        placement="bottom-end"
        open={open}
        anchorEl={anchorRef.current}
        role={undefined}
        transition
        disablePortal
        popperOptions={{ modifiers: [{ name: 'offset', options: { offset: [0, 9] } }] }}
      >
        {({ TransitionProps }) => (
          <Transitions type="grow" position="top-right" in={open} {...TransitionProps}>
            <Paper sx={{ boxShadow: 4, width: 260, minWidth: 220 }}>
              <ClickAwayListener onClickAway={handleClose}>
                <MainCard elevation={0} border={false} content={false}>
                  {/* User info */}
                  <Box sx={{ px: 2.5, pt: 2.5, pb: 1.5 }}>
                    <Stack direction="row" sx={{ gap: 1.25, alignItems: 'center', justifyContent: 'space-between' }}>
                      <Stack direction="row" sx={{ gap: 1.25, alignItems: 'center' }}>
                        {user?.avatarUrl ? (
                          <Avatar alt={user.name} src={user.avatarUrl} size="sm" />
                        ) : (
                          <Avatar size="sm"><UserOutlined /></Avatar>
                        )}
                        <Stack>
                          <Typography variant="h6">{user?.name ?? 'Usuário'}</Typography>
                          {user?.role && (
                            <Typography variant="body2" color="text.secondary">
                              {user.role}
                            </Typography>
                          )}
                        </Stack>
                      </Stack>
                      <Tooltip title={t('profile.logout')}>
                        <IconButton
                          size="large"
                          sx={{ color: 'text.primary' }}
                          aria-label={t('profile.logout')}
                          onClick={() => { onLogout?.(); closeMenu(); }}
                        >
                          <LogoutOutlined />
                        </IconButton>
                      </Tooltip>
                    </Stack>
                  </Box>

                  <Divider />

                  {/* Menu — spec: RF-00-UX-DEFAULT-APP-SHELL
                      Ordem: Minha Conta → Mudar de Conta → Sair → [divider] → Tema → Idioma → Config → [divider] → Ajuda */}
                  <List component="nav" sx={{ p: 0, '& .MuiListItemIcon-root': { minWidth: 32 } }}>
                    <ListItemButton onClick={() => { onAccount?.(); closeMenu(); }}>
                      <ListItemIcon><UserOutlined /></ListItemIcon>
                      <ListItemText primary={t('profile.myAccount')} />
                    </ListItemButton>

                    <ListItemButton onClick={() => { onSwitchAccount?.(); closeMenu(); }}>
                      <ListItemIcon><SwapOutlined /></ListItemIcon>
                      <ListItemText primary={t('profile.switchAccount')} />
                    </ListItemButton>

                    <ListItemButton onClick={() => { onLogout?.(); closeMenu(); }} sx={{ color: 'error.main' }}>
                      <ListItemIcon sx={{ color: 'error.main' }}><LogoutOutlined /></ListItemIcon>
                      <ListItemText primary={t('profile.logout')} />
                    </ListItemButton>

                    <Divider />

                    {/* Tema — RadioGroup com 3 opções */}
                    <ThemeToggleMenuItem onClose={closeMenu} />

                    {/* Idioma — RadioGroup com idiomas disponíveis */}
                    <LanguageSwitcher onClose={closeMenu} />

                    <ListItemButton onClick={() => { onSettings?.(); closeMenu(); }}>
                      <ListItemIcon><SettingOutlined /></ListItemIcon>
                      <ListItemText primary={t('profile.settings')} />
                    </ListItemButton>

                    <Divider />

                    <ListItemButton onClick={() => { onHelp?.(); closeMenu(); }}>
                      <ListItemIcon><QuestionCircleOutlined /></ListItemIcon>
                      <ListItemText primary={t('profile.help')} />
                    </ListItemButton>
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
