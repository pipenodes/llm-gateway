import { styled } from '@mui/material/styles';
import Box from '@mui/material/Box';

const DrawerHeaderStyled = styled(Box, {
  shouldForwardProp: (prop) => prop !== 'open'
})<{ open?: boolean }>(({ theme }) => ({
  ...theme.mixins.toolbar,
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  paddingLeft: theme.spacing(0),
  variants: [
    {
      props: ({ open }: { open?: boolean }) => open,
      style: { justifyContent: 'flex-start', paddingLeft: theme.spacing(3) }
    }
  ]
}));

export default DrawerHeaderStyled;
