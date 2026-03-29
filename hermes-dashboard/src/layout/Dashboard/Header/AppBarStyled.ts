import { styled } from '@mui/material/styles';
import AppBar from '@mui/material/AppBar';

import { DRAWER_WIDTH } from 'config';

const AppBarStyled = styled(AppBar, {
  shouldForwardProp: (prop) => prop !== 'open'
})<{ open?: boolean }>(({ theme }) => ({
  zIndex: theme.zIndex.drawer + 1,
  // spec: AppBar transition — accelerate on close, decelerate on open
  transition: theme.transitions.create(['width', 'margin'], {
    easing: theme.transitions.easing.easeIn,
    duration: theme.transitions.duration.leavingScreen
  }),
  variants: [
    {
      props: ({ open }: { open?: boolean }) => !open,
      style: { width: `calc(100% - ${theme.spacing(7.5)})` }
    },
    {
      props: ({ open }: { open?: boolean }) => open,
      style: {
        marginLeft: DRAWER_WIDTH,
        width: `calc(100% - ${DRAWER_WIDTH}px)`,
        transition: theme.transitions.create(['width', 'margin'], {
          easing: theme.transitions.easing.easeOut,
          duration: theme.transitions.duration.enteringScreen
        })
      }
    }
  ]
}));

export default AppBarStyled;
