import getColors from 'utils/getColors';
import getShadow from 'utils/getShadow';
import { tv } from 'utils/themeVars';
import type { Theme } from '@mui/material/styles';

function getColor({ variant, theme }: { variant: string; theme: Theme }) {
  const colors = getColors(theme, variant);
  const shadows = getShadow(theme, variant);
  return {
    '&:hover .MuiOutlinedInput-notchedOutline': { borderColor: colors.light },
    '&.Mui-focused': {
      boxShadow: shadows,
      '& .MuiOutlinedInput-notchedOutline': { border: '1px solid', borderColor: colors.light }
    }
  };
}

export default function OutlinedInput(theme: Theme) {
  const p = tv(theme);
  return {
    MuiOutlinedInput: {
      styleOverrides: {
        input: { padding: '10.5px 14px 10.5px 12px' },
        notchedOutline: { borderColor: p.grey[300] },
        root: {
          ...getColor({ variant: 'primary', theme }),
          '&.Mui-error': { ...getColor({ variant: 'error', theme }) }
        },
        inputSizeSmall: { padding: '7.5px 8px 7.5px 12px' },
        inputMultiline: { padding: 0 },
        colorSecondary: getColor({ variant: 'secondary', theme }),
        colorError: getColor({ variant: 'error', theme }),
        colorWarning: getColor({ variant: 'warning', theme }),
        colorInfo: getColor({ variant: 'info', theme }),
        colorSuccess: getColor({ variant: 'success', theme })
      }
    }
  };
}
