import { useTheme } from '@mui/material/styles';
import Typography from '@mui/material/Typography';
import { tv } from 'utils/themeVars';

// Placeholder — substitua pelo logo real do produto
export default function LogoMain() {
  const theme = useTheme();
  const p = tv(theme);
  return (
    <Typography
      variant="h5"
      noWrap
      sx={{ color: p.primary.main, fontWeight: 700, letterSpacing: 1 }}
    >
      APP
    </Typography>
  );
}
