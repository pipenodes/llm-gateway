import React from 'react';
import type { ReactNode, Ref } from 'react';

import Collapse from '@mui/material/Collapse';
import Fade from '@mui/material/Fade';
import Grow from '@mui/material/Grow';
import Slide from '@mui/material/Slide';
import Zoom from '@mui/material/Zoom';
import Box from '@mui/material/Box';

type Position = 'top-left' | 'top-right' | 'top' | 'bottom-left' | 'bottom-right' | 'bottom';
type TransitionType = 'grow' | 'collapse' | 'fade' | 'slide' | 'zoom';
type SlideDirection = 'up' | 'down' | 'left' | 'right';

interface TransitionsProps {
  children: ReactNode;
  position?: Position;
  type?: TransitionType;
  direction?: SlideDirection;
  ref?: Ref<HTMLDivElement>;
  in?: boolean;
  [key: string]: unknown;
}

const originMap: Record<Position, string> = {
  'top-left': '0 0 0',
  'top-right': 'top right',
  'top': 'top',
  'bottom-left': 'bottom left',
  'bottom-right': 'bottom right',
  'bottom': 'bottom'
};

export default function Transitions({
  children,
  position = 'top-left',
  type = 'grow',
  direction = 'up',
  ref,
  ...others
}: TransitionsProps) {
  const positionSX = { transformOrigin: originMap[position] ?? '0 0 0' };

  return (
    <Box ref={ref}>
      {type === 'grow' && (
        <Grow {...others} timeout={{ appear: 0, enter: 200, exit: 150 }}>
          <Box sx={positionSX}>{children}</Box>
        </Grow>
      )}
      {type === 'collapse' && (
        <Collapse {...others} sx={positionSX}>
          {children}
        </Collapse>
      )}
      {type === 'fade' && (
        <Fade {...others} timeout={{ appear: 0, enter: 200, exit: 150 }}>
          <Box sx={positionSX}>{children}</Box>
        </Fade>
      )}
      {type === 'slide' && (
        <Slide {...others} timeout={{ appear: 0, enter: 200, exit: 150 }} direction={direction}>
          <Box sx={positionSX}>{children}</Box>
        </Slide>
      )}
      {type === 'zoom' && children && (
        <Zoom {...others}>
          <Box sx={positionSX}>{children}</Box>
        </Zoom>
      )}
    </Box>
  );
}

export function PopupTransition({ children, ref, ...props }: { children?: ReactNode; ref?: Ref<HTMLElement>;[key: string]: unknown }) {
  return (
    <Zoom ref={ref as never} timeout={200} {...props}>
      {children as React.ReactElement ?? <span />}
    </Zoom>
  );
}
