import { useEffect } from 'react';
import { Outlet, useMatches } from 'react-router-dom';
import { useTranslation } from 'react-i18next';

import useMediaQuery from '@mui/material/useMediaQuery';
import Toolbar from '@mui/material/Toolbar';
import Box from '@mui/material/Box';
import type { Theme } from '@mui/material/styles';

import Drawer from './Drawer';
import Header from './Header';
import Footer from './Footer';
import Loader from 'components/Loader';
import Breadcrumbs from 'components/@extended/Breadcrumbs';
import ImpersonationBanner from 'components/ImpersonationBanner';

import { handlerDrawerOpen, useGetMenuMaster } from 'api/menu';
import { useFocusOnRouteChange } from 'hooks/useFocusOnRouteChange';

// Tipagem do handle de rota para títulos descritivos por página
interface RouteHandle {
  title?: string; // chave i18n ou string literal
}

export default function DashboardLayout() {
  const { menuMasterLoading } = useGetMenuMaster();
  const downXL = useMediaQuery((theme: Theme) => theme.breakpoints.down('xl'));
  const { t, i18n } = useTranslation();

  // a11y: move foco para #main-content a cada troca de rota (WCAG 2.4.3)
  useFocusOnRouteChange();

  // Título por rota — lê o handle.title da rota ativa mais específica
  const matches = useMatches();
  useEffect(() => {
    const leafHandle = [...matches].reverse().find((m) => (m.handle as RouteHandle)?.title);
    const routeTitle = leafHandle ? (leafHandle.handle as RouteHandle).title : undefined;
    const appName = i18n.exists('appName') ? t('appName') : 'App';
    document.title = routeTitle ? `${t(routeTitle)} | ${appName}` : appName;
  }, [matches, t, i18n]);

  useEffect(() => {
    handlerDrawerOpen(!downXL);
  }, [downXL]);

  if (menuMasterLoading) return <Loader />;

  return (
    <Box sx={{ display: 'flex', width: '100%' }}>
      <Header />
      <Drawer />

      <Box
        component="main"
        id="main-content"
        // tabIndex={-1} permite receber foco programático sem aparecer no tab order
        tabIndex={-1}
        sx={{
          width: 'calc(100% - 260px)',
          flexGrow: 1,
          p: { xs: 2, sm: 3 },
          // Remove outline visível no foco programático (ainda acessível a ATs)
          outline: 'none'
        }}
        aria-label="Conteúdo principal"
      >
        <Toolbar sx={{ mt: 'inherit' }} />
        <Box
          sx={{
            px: { xs: 0, sm: 2 },
            position: 'relative',
            minHeight: 'calc(100vh - 110px)',
            display: 'flex',
            flexDirection: 'column'
          }}
        >
          <ImpersonationBanner />
          <Breadcrumbs />
          <Outlet />
          <Footer />
        </Box>
      </Box>
    </Box>
  );
}
