import { createBrowserRouter } from 'react-router-dom';

import MainRoutes from './MainRoutes';
import LoginRoutes from './LoginRoutes';

const router = createBrowserRouter(
  [MainRoutes, LoginRoutes],
  { basename: import.meta.env.VITE_APP_BASE_NAME ?? '/' }
);

export default router;
