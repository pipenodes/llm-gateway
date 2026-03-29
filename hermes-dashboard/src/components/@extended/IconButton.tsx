import type { ReactNode, Ref } from 'react';
import MuiIconButton from '@mui/material/IconButton';
import { styled } from '@mui/material/styles';
import type { Theme } from '@mui/material/styles';

import getColors from 'utils/getColors';
import getShadow from 'utils/getShadow';
import { tv } from 'utils/themeVars';
import { withAlpha } from 'utils/colorUtils';

type IconButtonVariant = 'text' | 'contained' | 'light' | 'shadow' | 'outlined' | 'dashed';
type IconButtonShape = 'square' | 'rounded';

function getColorStyle({ variant, theme, color }: { variant: IconButtonVariant; theme: Theme; color: string }) {
  const colors = getColors(theme, color);
  const { lighter, light, dark, main, contrastText } = colors;
  const shadows = getShadow(theme, `${color}Button`);

  const commonShadow = {
    '&::after': { boxShadow: `0 0 6px 6px ${withAlpha(main, 0.9)}` },
    '&:active::after': { boxShadow: `0 0 0 0 ${withAlpha(main, 0.9)}` },
    '&:focus-visible': { outline: `2px solid ${dark}`, outlineOffset: 2 }
  };

  switch (variant) {
    case 'contained':
      return { color: contrastText, background: main, '&:hover': { background: dark }, ...commonShadow };
    case 'light':
      return { color: main, background: lighter ?? light, '&:hover': { background: withAlpha(light, 0.5) }, ...commonShadow };
    case 'shadow':
      return {
        boxShadow: shadows,
        color: contrastText,
        background: main,
        '&:hover': { boxShadow: 'none', background: dark },
        ...commonShadow
      };
    case 'outlined':
      return { '&:hover': { background: 'transparent', color: dark, borderColor: dark }, ...commonShadow };
    case 'dashed':
      return { background: lighter ?? light, '&:hover': { color: dark, borderColor: dark }, ...commonShadow };
    case 'text':
    default:
      return {
        '&:hover': { color: dark, background: color === 'secondary' ? withAlpha(light, 0.1) : (lighter ?? light) },
        ...commonShadow
      };
  }
}

const IconButtonStyle = styled(MuiIconButton, {
  shouldForwardProp: (prop) => prop !== 'variant' && prop !== 'shape'
})<{ variant?: IconButtonVariant; shape?: IconButtonShape; color?: string }>(({ theme, color = 'primary', variant = 'text' }) => {
  const p = tv(theme);
  return {
    position: 'relative',
    '::after': {
      content: '""',
      display: 'block',
      position: 'absolute',
      left: 0,
      top: 0,
      width: '100%',
      height: '100%',
      borderRadius: 4,
      opacity: 0,
      transition: 'all 0.1s'
    },
    ':active::after': {
      position: 'absolute',
      borderRadius: 4,
      left: 0,
      top: 0,
      opacity: 1,
      transition: '0s'
    },
    ...getColorStyle({ variant, theme, color }),
    variants: [
      {
        props: { shape: 'rounded' as IconButtonShape },
        style: { borderRadius: '50%', '::after': { borderRadius: '50%' }, ':active::after': { borderRadius: '50%' } }
      },
      { props: { variant: 'outlined' as IconButtonVariant }, style: { border: '1px solid', borderColor: 'inherit' } },
      { props: { variant: 'dashed' as IconButtonVariant }, style: { border: '1px dashed', borderColor: 'inherit' } },
      {
        props: ({ variant: v }: { variant?: string }) => v !== 'text',
        style: {
          '&.Mui-disabled': {
            background: p.grey[200],
            '&:hover': { background: p.grey[200], color: p.grey[300], borderColor: 'inherit' }
          }
        }
      }
    ]
  };
});

interface IconButtonProps {
  variant?: IconButtonVariant;
  shape?: IconButtonShape;
  children?: ReactNode;
  color?: string;
  ref?: Ref<HTMLButtonElement>;
  [key: string]: unknown;
}

function IconButton({ variant = 'text', shape = 'square', children, color = 'primary', ref, ...others }: IconButtonProps) {
  return (
    <IconButtonStyle ref={ref as never} disableRipple variant={variant} shape={shape} color={color as never} {...others}>
      {children}
    </IconButtonStyle>
  );
}

IconButton.displayName = 'IconButton';
export default IconButton;
