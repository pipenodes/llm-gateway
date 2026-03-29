import { Suspense } from 'react';
import type { ComponentType, ReactNode } from 'react';

import Loader from './Loader';

// Loadable — lazy loading de páginas com fallback configurável.
// Por padrão usa LinearProgress (Loader) no topo — adequado para transições de rota.
// Para páginas com skeleton específico, passe o componente de skeleton como segundo argumento.
//
// Exemplo com skeleton customizado:
//   const DashboardPage = Loadable(lazy(() => import('pages/dashboard')), <SkeletonDashboard />);

const Loadable = <P extends object>(
  Component: ComponentType<P>,
  fallback?: ReactNode
) =>
  function LoadableWrapper(props: P) {
    return (
      <Suspense fallback={fallback ?? <Loader />}>
        <Component {...props} />
      </Suspense>
    );
  };

export default Loadable;
