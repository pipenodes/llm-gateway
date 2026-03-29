import { useMemo } from 'react';

import useMediaQuery from '@mui/material/useMediaQuery';
import AppBar from '@mui/material/AppBar';
import Toolbar from '@mui/material/Toolbar';
import type { Theme } from '@mui/material/styles';

import AppBarStyled from './AppBarStyled';
import HeaderContent from './HeaderContent';
import IconButton from 'components/@extended/IconButton';

import { handlerDrawerOpen, useGetMenuMaster } from 'api/menu';
import { DRAWER_WIDTH, MINI_DRAWER_WIDTH } from 'config';

import MenuFoldOutlined from '@ant-design/icons/MenuFoldOutlined';
import MenuUnfoldOutlined from '@ant-design/icons/MenuUnfoldOutlined';

export default function Header() {
  const downLG = useMediaQuery((theme: Theme) => theme.breakpoints.down('lg'));

  const { menuMaster } = useGetMenuMaster();
  const drawerOpen = menuMaster.isDashboardDrawerOpened;

  const headerContent = useMemo(() => <HeaderContent />, []);

  const mainHeader = (
    <Toolbar>
      <IconButton
        aria-label={drawerOpen ? 'Fechar menu' : 'Abrir menu'}
        onClick={() => handlerDrawerOpen(!drawerOpen)}
        edge="start"
        color="secondary"
        variant="light"
        sx={{
          color: 'text.primary',
          bgcolor: drawerOpen ? 'transparent' : 'grey.100',
          ml: { xs: 0, lg: -2 }
        }}
      >
        {!drawerOpen ? <MenuUnfoldOutlined /> : <MenuFoldOutlined />}
      </IconButton>
      {headerContent}
    </Toolbar>
  );

  const appBar = {
    position: 'fixed' as const,
    color: 'inherit' as const,
    elevation: 0,
    sx: {
      borderBottom: '1px solid',
      borderBottomColor: 'divider',
      zIndex: 1200,
      width: {
        xs: '100%',
        lg: drawerOpen
          ? `calc(100% - ${DRAWER_WIDTH}px)`
          : `calc(100% - ${MINI_DRAWER_WIDTH}px)`
      }
    }
  };

  return (
    <>
      {!downLG ? (
        <AppBarStyled open={drawerOpen} {...appBar}>
          {mainHeader}
        </AppBarStyled>
      ) : (
        <AppBar {...appBar}>{mainHeader}</AppBar>
      )}
    </>
  );
}
