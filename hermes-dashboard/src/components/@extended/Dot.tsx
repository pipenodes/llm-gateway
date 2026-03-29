import type { SxProps, Theme } from '@mui/material/styles';
import { useTheme } from '@mui/material/styles';
import Box from '@mui/material/Box';

import getColors from 'utils/getColors';

interface DotProps {
  color?: string;
  size?: number;
  variant?: 'filled' | 'outlined';
  sx?: SxProps<Theme>;
  [key: string]: unknown;
}

export default function Dot({ color, size, variant, sx, ...rest }: DotProps) {
  const theme = useTheme();
  const colors = getColors(theme, color ?? 'primary');
  const { main } = colors;

  return (
    <Box
      {...rest}
      sx={{
        width: size ?? 8,
        height: size ?? 8,
        borderRadius: '50%',
        ...(variant === 'outlined'
          ? { border: `1px solid ${main}` }
          : { bgcolor: main }),
        ...sx
      }}
    />
  );
}
