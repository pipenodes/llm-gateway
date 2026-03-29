import { tv } from 'utils/themeVars';
import type { Theme } from '@mui/material/styles';

export default function ListItemButton(theme: Theme) {
  const p = tv(theme);
  return {
    MuiListItemButton: {
      styleOverrides: {
        root: {
          '&.Mui-selected': {
            color: p.primary.main,
            '& .MuiListItemIcon-root': { color: p.primary.main }
          }
        }
      }
    }
  };
}
