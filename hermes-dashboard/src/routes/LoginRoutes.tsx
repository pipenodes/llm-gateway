import { lazy } from 'react';

import Loadable from 'components/Loadable';
import AuthLayout from 'layout/Auth';

const LoginPage = Loadable(lazy(() => import('pages/auth/Login')));

const LoginRoutes = {
  path: '/',
  element: <AuthLayout />,
  children: [
    {
      path: 'login',
      element: <LoginPage />
    }
  ]
};

export default LoginRoutes;
