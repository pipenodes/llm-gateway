import { forwardRef } from 'react';
import type { ReactNode } from 'react';
import type { SxProps, Theme } from '@mui/material/styles';
import Box from '@mui/material/Box';
import SimpleBarReact from 'simplebar-react';

interface SimpleBarProps {
  children?: ReactNode;
  sx?: SxProps<Theme>;
}

// simplebar-react v3 types conflict with some TS strict settings; cast is intentional
const SimpleBarComponent = SimpleBarReact as unknown as React.ComponentType<{
  scrollableNodeProps?: { ref?: React.Ref<HTMLDivElement> };
  children?: ReactNode;
}>;

import React from 'react';

const SimpleBar = forwardRef<HTMLDivElement, SimpleBarProps>(({ children, sx }, ref) => {
  return (
    <Box sx={{ maxHeight: '100%', ...sx }}>
      <SimpleBarComponent scrollableNodeProps={{ ref }}>
        {children}
      </SimpleBarComponent>
    </Box>
  );
});

SimpleBar.displayName = 'SimpleBar';

export default SimpleBar;
