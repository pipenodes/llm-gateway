import { tv } from 'utils/themeVars';
import type { Theme } from '@mui/material/styles';

export default function TableHead(theme: Theme) {
  const p = tv(theme);
  return {
    MuiTableHead: {
      styleOverrides: {
        root: {
          backgroundColor: p.grey[50],
          borderTop: '1px solid',
          borderTopColor: p.divider,
          borderBottom: '2px solid',
          borderBottomColor: p.divider
        }
      }
    }
  };
}
