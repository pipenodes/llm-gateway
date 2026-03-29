import type { ReactNode } from 'react';
import { motion, useCycle } from 'framer-motion';

interface ScaleConfig { hover?: number; tap?: number }

interface AnimateButtonProps {
  children: ReactNode;
  type?: 'scale' | 'slide' | 'rotate';
  direction?: 'up' | 'down' | 'left' | 'right';
  offset?: number;
  scale?: number | ScaleConfig;
}

export default function AnimateButton({
  children,
  type = 'scale',
  direction = 'right',
  offset = 10,
  scale = { hover: 1.05, tap: 0.954 }
}: AnimateButtonProps) {
  const offset1 = direction === 'up' || direction === 'left' ? offset : 0;
  const offset2 = direction === 'right' || direction === 'down' ? offset : 0;

  const [x, cycleX] = useCycle(offset1, offset2);
  const [y, cycleY] = useCycle(offset1, offset2);

  const scaleConfig: ScaleConfig = typeof scale === 'number' ? { hover: scale, tap: scale } : scale;

  switch (type) {
    case 'rotate':
      return (
        <motion.div animate={{ rotate: 360 }} transition={{ repeat: Infinity, repeatType: 'loop', duration: 2, repeatDelay: 0 }}>
          {children}
        </motion.div>
      );
    case 'slide':
      if (direction === 'up' || direction === 'down') {
        return (
          <motion.div animate={{ y: y ?? 0 }} onHoverEnd={() => cycleY()} onHoverStart={() => cycleY()}>
            {children}
          </motion.div>
        );
      }
      return (
        <motion.div animate={{ x: x ?? 0 }} onHoverEnd={() => cycleX()} onHoverStart={() => cycleX()}>
          {children}
        </motion.div>
      );
    case 'scale':
    default:
      return (
        <motion.div whileHover={{ scale: scaleConfig.hover }} whileTap={{ scale: scaleConfig.tap }}>
          {children}
        </motion.div>
      );
  }
}
