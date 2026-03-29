import type { Theme, PaletteColor } from '@mui/material/styles';

type ColorKey = 'primary' | 'secondary' | 'error' | 'warning' | 'info' | 'success';

// theme.vars is always defined when using colorSchemes — cast is safe
function getVarsPalette(theme: Theme) {
  return (theme.vars ?? theme) as unknown as { palette: Record<ColorKey, PaletteColor> };
}

export default function getColors(theme: Theme, color: string): PaletteColor {
  const palette = getVarsPalette(theme).palette;
  const key = color as ColorKey;
  return palette[key] ?? palette.primary;
}
