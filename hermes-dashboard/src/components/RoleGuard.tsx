import type { ReactNode } from 'react';

interface RoleGuardProps {
  roles?: string[];
  children: ReactNode;
  fallback?: ReactNode;
}

export default function RoleGuard({ children }: RoleGuardProps) {
  return <>{children}</>;
}
