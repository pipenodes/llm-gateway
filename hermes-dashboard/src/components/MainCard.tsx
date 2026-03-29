import type { ReactNode, Ref } from 'react';
import type { SxProps, Theme } from '@mui/material/styles';

import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import CardHeader from '@mui/material/CardHeader';
import Divider from '@mui/material/Divider';

import { tv } from 'utils/themeVars';

interface MainCardProps {
  border?: boolean;
  boxShadow?: boolean;
  children?: ReactNode;
  subheader?: string | ReactNode;
  content?: boolean;
  contentSX?: SxProps<Theme>;
  darkTitle?: boolean;
  divider?: boolean;
  elevation?: number;
  secondary?: ReactNode;
  shadow?: string;
  sx?: SxProps<Theme>;
  title?: string | ReactNode;
  codeHighlight?: boolean;
  modal?: boolean;
  ref?: Ref<HTMLDivElement>;
  [key: string]: unknown;
}

export default function MainCard({
  border = true,
  boxShadow,
  children,
  subheader,
  content = true,
  contentSX = {},
  darkTitle,
  divider = true,
  elevation,
  secondary,
  shadow,
  sx = {},
  title,
  codeHighlight = false,
  modal = false,
  ref,
  ...others
}: MainCardProps) {
  return (
    <Card
      elevation={elevation ?? 0}
      sx={(theme) => {
        const p = tv(theme);
        const shadows = (theme as unknown as { customShadows?: Record<string, string> }).customShadows;
        return {
          position: 'relative',
          ...(border && { border: `1px solid ${(p.grey as unknown as Record<string, string>)['A800'] ?? p.divider}` }),
          borderRadius: 1,
          boxShadow: boxShadow && !border ? (shadow ?? shadows?.z1 ?? 'none') : 'inherit',
          ':hover': { boxShadow: boxShadow ? (shadow ?? shadows?.z1 ?? 'none') : 'inherit' },
          ...(codeHighlight && {
            '& pre': {
              margin: 0,
              padding: '12px !important',
              fontFamily: theme.typography.fontFamily,
              fontSize: '0.75rem'
            }
          }),
          ...(modal && {
            position: 'absolute',
            top: '50%',
            left: '50%',
            transform: 'translate(-50%, -50%)',
            width: { xs: `calc(100% - 50px)`, sm: 'auto' },
            maxWidth: 768
          }),
          ...(typeof sx === 'function' ? sx(theme) : (sx ?? {}))
        };
      }}
      ref={ref}
      {...others}
    >
      {!darkTitle && title && (
        <CardHeader
          sx={{ p: 2.5 }}
          slotProps={{
            title: { variant: darkTitle ? 'h4' : 'subtitle1' },
            action: { sx: { m: '0px auto', alignSelf: 'center' } }
          }}
          title={title}
          action={secondary}
          subheader={subheader}
        />
      )}

      {title && divider && <Divider />}

      {content && <CardContent sx={contentSX}>{children}</CardContent>}
      {!content && children}
    </Card>
  );
}
