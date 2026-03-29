import getColors from 'utils/getColors';
import { tv } from 'utils/themeVars';
import type { Theme } from '@mui/material/styles';

function getColorStyle({ color, theme }: { color: string; theme: Theme }) {
  const colors = getColors(theme, color);
  return { color: colors.main, backgroundColor: colors.lighter };
}

export default function Badge(theme: Theme) {
  const defaultLightBadge = getColorStyle({ color: 'primary', theme });
  const p = tv(theme);
  return {
    MuiBadge: {
      styleOverrides: {
        standard: {
          minWidth: theme.spacing(2),
          height: theme.spacing(2),
          padding: theme.spacing(0.5)
        },
        light: {
          ...defaultLightBadge,
          '&.MuiBadge-colorPrimary': getColorStyle({ color: 'primary', theme }),
          '&.MuiBadge-colorSecondary': getColorStyle({ color: 'secondary', theme }),
          '&.MuiBadge-colorError': getColorStyle({ color: 'error', theme }),
          '&.MuiBadge-colorInfo': getColorStyle({ color: 'info', theme }),
          '&.MuiBadge-colorSuccess': getColorStyle({ color: 'success', theme }),
          '&.MuiBadge-colorWarning': getColorStyle({ color: 'warning', theme })
        }
      }
    }
  };
  void p; // tv used for type-safety guard if needed
}
