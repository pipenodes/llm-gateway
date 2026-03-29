import { tv } from 'utils/themeVars';
import type { Theme } from '@mui/material/styles';

export default function InputLabel(theme: Theme) {
  const p = tv(theme);
  return {
    MuiInputLabel: {
      styleOverrides: {
        root: { color: p.grey[600] },
        outlined: {
          lineHeight: '1rem',
          top: -4,
          '&.MuiInputLabel-sizeSmall': { lineHeight: '1em' },
          '&.MuiInputLabel-shrink': {
            background: p.background.paper,
            padding: '0 8px',
            marginLeft: -6,
            top: 2,
            lineHeight: '1rem'
          }
        }
      }
    }
  };
}
