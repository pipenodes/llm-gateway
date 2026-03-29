import { withAlpha } from 'utils/colorUtils';
import { tv } from 'utils/themeVars';
import getColors from 'utils/getColors';
import type { Theme } from '@mui/material/styles';

export default function Tab(theme: Theme) {
  const p = tv(theme);
  const primaryColors = getColors(theme, 'primary');
  const secondaryColors = getColors(theme, 'secondary');
  return {
    MuiTab: {
      styleOverrides: {
        root: {
          minHeight: 46,
          color: p.text.primary,
          borderRadius: 4,
          '&:hover': {
            backgroundColor: withAlpha(primaryColors.lighter ?? primaryColors.light, 0.6),
            color: primaryColors.main
          },
          '&:focus-visible': {
            borderRadius: 4,
            outline: `2px solid ${secondaryColors.dark}`,
            outlineOffset: -3
          }
        }
      }
    }
  };
}
