import { useTheme } from '@mui/material/styles';
import Box from '@mui/material/Box';
import { tv } from 'utils/themeVars';

// Placeholder — substitua pelo ícone real do produto
export default function LogoIcon() {
  const theme = useTheme();
  const p = tv(theme);
  return (
    <Box
      sx={{
        width: 32,
        height: 32,
        borderRadius: 1,
        bgcolor: p.primary.main,
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        color: '#fff',
        fontWeight: 700,
        fontSize: '0.875rem'
      }}
    >
      A
    </Box>
  );
}
