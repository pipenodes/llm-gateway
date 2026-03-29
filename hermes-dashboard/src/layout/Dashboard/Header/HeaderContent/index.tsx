import { useNavigate } from 'react-router-dom';
import useMediaQuery from '@mui/material/useMediaQuery';
import Box from '@mui/material/Box';
import type { Theme } from '@mui/material/styles';

import Search from './Search';
import Profile from './Profile';
import MobileSection from './MobileSection';
import { useAuth } from 'contexts/AuthContext';

export default function HeaderContent() {
  const downLG = useMediaQuery((theme: Theme) => theme.breakpoints.down('lg'));
  const { logout } = useAuth();
  const navigate = useNavigate();

  const profileProps = {
    user: { name: 'Admin', role: 'Gateway Admin' },
    onLogout: logout,
    onSettings: () => navigate('/settings'),
    onHelp: () => {}
  };

  return (
    <>
      {!downLG && <Search />}
      {downLG && <Box sx={{ width: '100%', ml: 1 }} />}

      {!downLG && <Profile {...profileProps} />}
      {downLG && <MobileSection />}
    </>
  );
}
