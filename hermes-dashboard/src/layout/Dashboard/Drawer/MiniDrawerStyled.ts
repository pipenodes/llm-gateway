import { styled } from '@mui/material/styles';
import Drawer from '@mui/material/Drawer';

import { DRAWER_WIDTH } from 'config';

const openedMixin = (theme: import('@mui/material/styles').Theme) => ({
  width: DRAWER_WIDTH,
  borderRight: '1px solid',
  borderRightColor: theme.palette.divider,
  // spec: drawer open 250ms decelerate — RF-00-UX-DEFAULT-MOTION
  transition: theme.transitions.create('width', {
    easing: theme.transitions.easing.easeOut,
    duration: theme.transitions.duration.enteringScreen
  }),
  overflowX: 'hidden',
  boxShadow: 'none'
});

const closedMixin = (theme: import('@mui/material/styles').Theme) => ({
  // spec: drawer close 200ms accelerate
  transition: theme.transitions.create('width', {
    easing: theme.transitions.easing.easeIn,
    duration: theme.transitions.duration.leavingScreen
  }),
  overflowX: 'hidden',
  width: theme.spacing(7.5),
  borderRight: 'none',
  boxShadow: (theme as unknown as { customShadows?: Record<string, string> }).customShadows?.z1 ?? 'none'
});

const MiniDrawerStyled = styled(Drawer, {
  shouldForwardProp: (prop) => prop !== 'open'
})(({ theme }) => ({
  width: DRAWER_WIDTH,
  flexShrink: 0,
  whiteSpace: 'nowrap',
  boxSizing: 'border-box',
  variants: [
    {
      props: ({ open }: { open?: boolean }) => open,
      style: { ...openedMixin(theme), '& .MuiDrawer-paper': openedMixin(theme) }
    },
    {
      props: ({ open }: { open?: boolean }) => !open,
      style: { ...closedMixin(theme), '& .MuiDrawer-paper': closedMixin(theme) }
    }
  ]
}));

export default MiniDrawerStyled;
