import { useEffect } from 'react';
import type { ReactNode } from 'react';

export default function ScrollTop({ children }: { children?: ReactNode }) {
  useEffect(() => {
    window.scrollTo({ top: 0, left: 0, behavior: 'smooth' });
  }, []);

  return children ?? null;
}
