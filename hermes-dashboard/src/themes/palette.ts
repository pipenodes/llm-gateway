import { presetPalettes } from '@ant-design/colors';

import ThemeOption from './theme';
import { extendPaletteWithChannels } from 'utils/colorUtils';

function buildGrey(): string[] {
  const greyPrimary = [
    '#ffffff', // 0
    '#fafafa', // 1
    '#f5f5f5', // 2
    '#f0f0f0', // 3
    '#d9d9d9', // 4
    '#bfbfbf', // 5
    '#8c8c8c', // 6
    '#595959', // 7
    '#262626', // 8
    '#141414', // 9
    '#000000'  // 10
  ];
  const greyAscent = ['#fafafa', '#bfbfbf', '#434343', '#1f1f1f'];
  const greyConstant = ['#fafafb', '#e6ebf1'];

  return [...greyPrimary, ...greyAscent, ...greyConstant];
}

export function buildPalette() {
  const colors = { ...presetPalettes, grey: buildGrey() } as Record<string, string[]>;
  const paletteColor = ThemeOption(colors as never);

  const commonColor = { common: { black: '#000', white: '#fff' } };
  const extended = extendPaletteWithChannels(paletteColor);
  const extendedCommon = extendPaletteWithChannels(commonColor);

  const grey = extended.grey as Record<string | number, string>;

  // ── LIGHT MODE ──────────────────────────────────────────────────────────
  const light = {
    mode: 'light' as const,
    ...extendedCommon,
    ...extended,
    text: {
      // spec: #141414 (grey[900]) para contraste WCAG AA
      primary: '#141414',
      secondary: grey[500],
      disabled: grey[400]
    },
    action: { disabled: grey[300] },
    divider: grey[200],
    background: {
      paper: grey[0],
      // spec: #f5f5f5
      default: '#f5f5f5'
    }
  };

  // ── DARK MODE ────────────────────────────────────────────────────────────
  // spec: RF-00-UX-DEFAULT-TOKENS.md
  const dark = {
    mode: 'dark' as const,
    ...extendedCommon,
    ...extended,
    text: {
      primary: '#ffffff',
      secondary: '#8c8c8c',
      disabled: '#434343'
    },
    action: { disabled: '#434343' },
    divider: '#303030',
    background: {
      paper: '#1f1f1f',
      default: '#141414'
    }
  };

  return { light, dark };
}
