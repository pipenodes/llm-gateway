import { alpha } from '@mui/material/styles';

export function hexToRgbChannel(hex: string): string {
  let cleaned = hex.replace(/^#/, '');

  if (cleaned.length === 3 || cleaned.length === 4) {
    cleaned = cleaned.split('').map((c) => c + c).join('');
  }

  if (cleaned.length !== 6 && cleaned.length !== 8) {
    throw new Error(`Invalid hex color: ${hex}`);
  }

  const r = parseInt(cleaned.substring(0, 2), 16);
  const g = parseInt(cleaned.substring(2, 4), 16);
  const b = parseInt(cleaned.substring(4, 6), 16);

  return `${r} ${g} ${b}`;
}

export function extendPaletteWithChannels<T extends Record<string, unknown>>(palette: T): T {
  const result = { ...palette } as Record<string, unknown>;

  Object.entries(palette).forEach(([k, v]) => {
    if (typeof v === 'string' && v.startsWith('#')) {
      result[`${k}Channel`] = hexToRgbChannel(v);
    } else if (typeof v === 'object' && v !== null) {
      result[k] = extendPaletteWithChannels(v as Record<string, unknown>);
    }
  });

  return result as T;
}

export function withAlpha(color: string, opacity: number): string {
  if (/^#|rgb|hsl|color/i.test(color)) {
    return alpha(color, opacity);
  }

  if (color.startsWith('var(')) {
    return color
      .replace(/(--[a-zA-Z0-9-]+)(.*)\)/, `$1Channel$2)`)
      .replace(/^var\((.+)\)$/, `rgba(var($1) / ${opacity})`);
  }

  return color;
}
