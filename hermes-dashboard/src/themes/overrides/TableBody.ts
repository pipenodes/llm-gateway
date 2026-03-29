import { tv } from 'utils/themeVars';
import type { Theme } from '@mui/material/styles';

export default function TableBody(theme: Theme) {
  const p = tv(theme);
  return {
    MuiTableBody: {
      styleOverrides: {
        root: {
          backgroundColor: p.background.paper,
          '& .MuiTableRow-root': {
            '&:hover': { backgroundColor: p.action.hover }
          }
        }
      }
    }
  };
}
