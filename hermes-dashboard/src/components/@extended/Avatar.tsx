import type { ReactNode } from 'react';
import { styled } from '@mui/material/styles';
import MuiAvatar from '@mui/material/Avatar';
import type { Theme } from '@mui/material/styles';

import getColors from 'utils/getColors';
import { tv } from 'utils/themeVars';

type AvatarType = 'filled' | 'outlined' | 'combined' | 'tonal';
type AvatarSize = 'badge' | 'xs' | 'sm' | 'md' | 'lg' | 'xl';

function getColorStyle({ theme, color, type }: { theme: Theme; color: string; type?: AvatarType }) {
  const colors = getColors(theme, color);
  const { lighter, light, main, contrastText } = colors;
  switch (type) {
    case 'filled': return { color: contrastText, background: main };
    case 'outlined': return { color: main, border: '1px solid', borderColor: main, background: 'transparent' };
    case 'combined': return { color: main, border: '1px solid', borderColor: light, background: lighter ?? light };
    default: return { color: main, background: lighter ?? light };
  }
}

function getSizeStyle(size: AvatarSize) {
  switch (size) {
    case 'badge': return { border: '2px solid', fontSize: '0.675rem', width: 20, height: 20 };
    case 'xs': return { fontSize: '0.75rem', width: 24, height: 24 };
    case 'sm': return { fontSize: '0.875rem', width: 32, height: 32 };
    case 'lg': return { fontSize: '1.2rem', width: 52, height: 52 };
    case 'xl': return { fontSize: '1.5rem', width: 64, height: 64 };
    case 'md':
    default: return { fontSize: '1rem', width: 40, height: 40 };
  }
}

const AvatarStyle = styled(MuiAvatar, {
  shouldForwardProp: (prop) => prop !== 'color' && prop !== 'type' && prop !== 'size'
})<{ color?: string; type?: AvatarType; size?: AvatarSize }>(({ theme, size = 'md', color = 'primary', type }) => ({
  ...getSizeStyle(size),
  ...getColorStyle({ theme, color, type }),
  variants: [
    {
      props: { size: 'badge' as AvatarSize },
      style: { borderColor: tv(theme).background.default }
    }
  ]
}));

interface AvatarProps {
  children?: ReactNode;
  color?: string;
  type?: AvatarType;
  size?: AvatarSize;
  [key: string]: unknown;
}

export default function Avatar({ children, color = 'primary', type, size = 'md', ...others }: AvatarProps) {
  return (
    <AvatarStyle color={color} type={type} size={size} {...others}>
      {children}
    </AvatarStyle>
  );
}
