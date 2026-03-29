import { tv } from 'utils/themeVars';
import type { Theme } from '@mui/material/styles';

export default function ListItemIcon(theme: Theme) {
  const p = tv(theme);
  return {
    MuiListItemIcon: {
      styleOverrides: {
        root: { minWidth: 24, color: p.text.primary }
      }
    }
  };
}
