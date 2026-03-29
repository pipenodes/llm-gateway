import { tv } from 'utils/themeVars';
import type { Theme } from '@mui/material/styles';

export default function Tooltip(theme: Theme) {
  const p = tv(theme);
  return {
    MuiTooltip: {
      styleOverrides: {
        tooltip: { color: p.background.paper }
      }
    }
  };
}
