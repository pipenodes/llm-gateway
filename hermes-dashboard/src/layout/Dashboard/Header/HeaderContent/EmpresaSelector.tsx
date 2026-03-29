import { useTranslation } from 'react-i18next';
import Button from '@mui/material/Button';
import ListItemText from '@mui/material/ListItemText';
import Menu from '@mui/material/Menu';
import MenuItem from '@mui/material/MenuItem';
import Typography from '@mui/material/Typography';
import Box from '@mui/material/Box';
import { useState, useRef, useMemo, useEffect } from 'react';
import ApartmentOutlined from '@ant-design/icons/ApartmentOutlined';
import { useEmpresaAtiva } from 'contexts/EmpresaAtivaContext';
import { useAuth } from 'contexts/AuthContext';

export default function EmpresaSelector() {
  const { t } = useTranslation();
  const { empresaId, setEmpresaId, empresas: allEmpresas, empresaAtiva } = useEmpresaAtiva();
  const { user } = useAuth();
  const [anchorEl, setAnchorEl] = useState<null | HTMLElement>(null);
  const buttonRef = useRef<HTMLButtonElement>(null);

  const empresas = useMemo(() => {
    if (!user) return [];
    if (user.isSuperAdmin) return allEmpresas;
    return allEmpresas.filter((e) =>
      user.empresas.some((m) => m.empresaId === e.empresaId && m.status === 'ativo')
    );
  }, [user, allEmpresas]);

  useEffect(() => {
    if (!empresaId || empresas.some((e) => e.empresaId === empresaId)) return;
    setEmpresaId(empresas[0]?.empresaId ?? null);
  }, [empresaId, empresas, setEmpresaId]);

  const handleOpen = () => setAnchorEl(buttonRef.current);
  const handleClose = () => setAnchorEl(null);

  const handleSelect = (id: string) => {
    setEmpresaId(id);
    handleClose();
  };

  if (empresas.length === 0) return null;

  return (
    <Box sx={{ mr: 1 }}>
      <Button
        ref={buttonRef}
        onClick={handleOpen}
        startIcon={<ApartmentOutlined style={{ fontSize: '1rem' }} />}
        sx={{ color: 'text.primary', textTransform: 'none' }}
        aria-label={t('empresas.activeCompany', { defaultValue: 'Empresa ativa' })}
      >
        <Typography variant="body2" noWrap sx={{ maxWidth: 160 }}>
          {empresaAtiva?.nome ?? t('empresas.noCompany', { defaultValue: 'Nenhuma' })}
        </Typography>
      </Button>
      <Menu
        anchorEl={anchorEl}
        open={!!anchorEl}
        onClose={handleClose}
        anchorOrigin={{ vertical: 'bottom', horizontal: 'left' }}
        transformOrigin={{ vertical: 'top', horizontal: 'left' }}
      >
        {empresas.map((emp) => (
          <MenuItem
            key={emp.empresaId}
            selected={emp.empresaId === empresaId}
            onClick={() => handleSelect(emp.empresaId)}
          >
            <ListItemText primary={emp.nome} />
          </MenuItem>
        ))}
      </Menu>
    </Box>
  );
}
