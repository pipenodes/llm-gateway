import { lazy } from 'react';

import Loadable from 'components/Loadable';
import DashboardLayout from 'layout/Dashboard';
import { ServerErrorRoute } from 'pages/errors/ServerError';
import NotFound from 'pages/errors/NotFound';
import AuthGuard from './AuthGuard';

const DashboardPage = Loadable(lazy(() => import('pages/dashboard')));
const KeysPage = Loadable(lazy(() => import('pages/keys')));
const UsagePage = Loadable(lazy(() => import('pages/usage')));
const CostsPage = Loadable(lazy(() => import('pages/costs')));
const AuditPage = Loadable(lazy(() => import('pages/audit')));
const PluginsPage = Loadable(lazy(() => import('pages/plugins')));
const PromptsPage = Loadable(lazy(() => import('pages/prompts')));
const ABTestingPage = Loadable(lazy(() => import('pages/ab-testing')));
const SessionsPage = Loadable(lazy(() => import('pages/sessions')));
const WebhooksPage = Loadable(lazy(() => import('pages/webhooks')));
const SecurityPage = Loadable(lazy(() => import('pages/security')));
const ModelsPage = Loadable(lazy(() => import('pages/models')));
const SettingsPage = Loadable(lazy(() => import('pages/settings')));
// Enterprise Security Pipeline pages (RF-32 to RF-35)
const GuardrailsPage = Loadable(lazy(() => import('pages/guardrails')));
const DataDiscoveryPage = Loadable(lazy(() => import('pages/discovery')));
const DLPPage = Loadable(lazy(() => import('pages/dlp')));
const FinOpsPage = Loadable(lazy(() => import('pages/finops')));

const MainRoutes = {
  path: '/',
  element: (
    <AuthGuard>
      <DashboardLayout />
    </AuthGuard>
  ),
  errorElement: <ServerErrorRoute />,
  children: [
    { index: true, element: <DashboardPage />, handle: { title: 'nav.dashboard' } },
    { path: 'dashboard', element: <DashboardPage />, handle: { title: 'nav.dashboard' } },
    { path: 'keys', element: <KeysPage />, handle: { title: 'nav.keys' } },
    { path: 'usage', element: <UsagePage />, handle: { title: 'nav.usage' } },
    { path: 'costs', element: <CostsPage />, handle: { title: 'nav.costs' } },
    { path: 'audit', element: <AuditPage />, handle: { title: 'nav.audit' } },
    { path: 'plugins', element: <PluginsPage />, handle: { title: 'nav.plugins' } },
    { path: 'prompts', element: <PromptsPage />, handle: { title: 'nav.prompts' } },
    { path: 'ab-testing', element: <ABTestingPage />, handle: { title: 'nav.abTesting' } },
    { path: 'sessions', element: <SessionsPage />, handle: { title: 'nav.sessions' } },
    { path: 'webhooks', element: <WebhooksPage />, handle: { title: 'nav.webhooks' } },
    { path: 'security', element: <SecurityPage />, handle: { title: 'nav.security' } },
    { path: 'models', element: <ModelsPage />, handle: { title: 'nav.models' } },
    { path: 'settings', element: <SettingsPage />, handle: { title: 'nav.settings' } },
    // Enterprise Security Pipeline
    { path: 'guardrails', element: <GuardrailsPage />, handle: { title: 'nav.guardrails' } },
    { path: 'discovery', element: <DataDiscoveryPage />, handle: { title: 'nav.discovery' } },
    { path: 'dlp', element: <DLPPage />, handle: { title: 'nav.dlp' } },
    { path: 'finops', element: <FinOpsPage />, handle: { title: 'nav.finops' } },
    { path: '*', element: <NotFound /> }
  ]
};

export default MainRoutes;
