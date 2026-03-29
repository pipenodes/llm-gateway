import { useState, useEffect, useCallback } from 'react';
import { useNavigate } from 'react-router-dom';

import Autocomplete from '@mui/material/Autocomplete';
import Dialog from '@mui/material/Dialog';
import DialogContent from '@mui/material/DialogContent';
import InputAdornment from '@mui/material/InputAdornment';
import TextField from '@mui/material/TextField';
import Typography from '@mui/material/Typography';
import Box from '@mui/material/Box';
import ListItem from '@mui/material/ListItem';
import ListItemIcon from '@mui/material/ListItemIcon';
import ListItemText from '@mui/material/ListItemText';

import SearchOutlined from '@ant-design/icons/SearchOutlined';
import DashboardOutlined from '@ant-design/icons/DashboardOutlined';
import TeamOutlined from '@ant-design/icons/TeamOutlined';
import BarChartOutlined from '@ant-design/icons/BarChartOutlined';
import AuditOutlined from '@ant-design/icons/AuditOutlined';
import UserOutlined from '@ant-design/icons/UserOutlined';
import SettingOutlined from '@ant-design/icons/SettingOutlined';
import { useAuth } from 'contexts/AuthContext';

interface CommandItem {
  id: string;
  label: string;
  description?: string;
  url: string;
  icon: React.ReactNode;
  roles?: string[];
}

const ALL_COMMANDS: CommandItem[] = [
  { id: 'dashboard', label: 'Dashboard', description: 'Visão geral do sistema', url: '/dashboard', icon: <DashboardOutlined /> },
  { id: 'users', label: 'Usuários', description: 'Gerenciar usuários', url: '/users', icon: <TeamOutlined />, roles: ['admin', 'manager'] },
  { id: 'reports', label: 'Relatórios', description: 'Visualizar relatórios', url: '/reports', icon: <BarChartOutlined />, roles: ['admin', 'manager'] },
  { id: 'audit', label: 'Auditoria', description: 'Log de ações do sistema', url: '/audit', icon: <AuditOutlined />, roles: ['admin'] },
  { id: 'profile', label: 'Meu Perfil', description: 'Editar suas informações', url: '/profile', icon: <UserOutlined /> },
  { id: 'settings', label: 'Configurações', description: 'Preferências do sistema', url: '/settings', icon: <SettingOutlined />, roles: ['admin'] },
];

interface CommandPaletteProps {
  open?: boolean;
  onClose?: () => void;
}

export default function CommandPalette({ open: controlledOpen, onClose }: CommandPaletteProps) {
  const [internalOpen, setInternalOpen] = useState(false);
  const open = controlledOpen ?? internalOpen;
  const navigate = useNavigate();
  const { user } = useAuth();

  const commands = ALL_COMMANDS.filter((cmd) => {
    if (!cmd.roles) return true;
    return user ? cmd.roles.includes(user.role) : false;
  });

  const handleOpen = useCallback(() => setInternalOpen(true), []);
  const handleClose = useCallback(() => { setInternalOpen(false); onClose?.(); }, [onClose]);

  // Atalho Ctrl+K / Cmd+K
  useEffect(() => {
    const handler = (e: KeyboardEvent) => {
      if ((e.ctrlKey || e.metaKey) && e.key === 'k') {
        e.preventDefault();
        setOpen((prev) => !prev);
      }
    };
    document.addEventListener('keydown', handler);
    return () => document.removeEventListener('keydown', handler);
  }, []);

  const handleSelect = (_: unknown, value: CommandItem | null) => {
    if (value) {
      navigate(value.url);
      handleClose();
    }
  };

  return (
    <>
      {/* Botão de ativação no Search do header */}
      <Dialog
        open={open}
        onClose={handleClose}
        maxWidth="sm"
        fullWidth
        PaperProps={{ sx: { mt: '10vh', verticalAlign: 'top', borderRadius: 2 } }}
        aria-label="Busca rápida"
      >
        <DialogContent sx={{ p: 0 }}>
          <Autocomplete<CommandItem>
            open
            options={commands}
            getOptionLabel={(o) => o.label}
            onChange={handleSelect}
            disablePortal
            freeSolo={false}
            renderInput={(params) => (
              <TextField
                {...params}
                autoFocus
                placeholder="Buscar página ou ação..."
                variant="outlined"
                slotProps={{
                  input: {
                    ...params.InputProps,
                    startAdornment: (
                      <InputAdornment position="start">
                        <SearchOutlined style={{ fontSize: '1.1rem' }} />
                      </InputAdornment>
                    ),
                    sx: { borderRadius: 2, '& fieldset': { border: 'none' } }
                  }
                }}
              />
            )}
            renderOption={(props, option) => (
              <ListItem {...props} key={option.id} sx={{ gap: 1 }}>
                <ListItemIcon sx={{ minWidth: 32, color: 'text.secondary', fontSize: '1rem' }}>
                  {option.icon}
                </ListItemIcon>
                <ListItemText
                  primary={<Typography variant="body2" fontWeight={600}>{option.label}</Typography>}
                  secondary={option.description}
                />
              </ListItem>
            )}
            ListboxProps={{ sx: { maxHeight: 320 } }}
          />
          <Box sx={{ px: 2, py: 0.75, borderTop: '1px solid', borderColor: 'divider', display: 'flex', gap: 2 }}>
            <Typography variant="caption" color="text.secondary">↵ selecionar</Typography>
            <Typography variant="caption" color="text.secondary">↑↓ navegar</Typography>
            <Typography variant="caption" color="text.secondary">Esc fechar</Typography>
          </Box>
        </DialogContent>
      </Dialog>

      {/* Botão oculto para teste automatizado */}
      <Box component="span" onClick={handleOpen} aria-label="Abrir busca rápida" sx={{ display: 'none' }} />
    </>
  );
}
