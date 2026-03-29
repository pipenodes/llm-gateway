import DashboardOutlined from '@ant-design/icons/DashboardOutlined';
import KeyOutlined from '@ant-design/icons/KeyOutlined';
import BarChartOutlined from '@ant-design/icons/BarChartOutlined';
import DollarOutlined from '@ant-design/icons/DollarOutlined';
import AuditOutlined from '@ant-design/icons/AuditOutlined';
import ApiOutlined from '@ant-design/icons/ApiOutlined';
import FileTextOutlined from '@ant-design/icons/FileTextOutlined';
import ExperimentOutlined from '@ant-design/icons/ExperimentOutlined';
import MessageOutlined from '@ant-design/icons/MessageOutlined';
import BellOutlined from '@ant-design/icons/BellOutlined';
import SafetyOutlined from '@ant-design/icons/SafetyOutlined';
import CloudServerOutlined from '@ant-design/icons/CloudServerOutlined';
import SettingOutlined from '@ant-design/icons/SettingOutlined';
import SecurityScanOutlined from '@ant-design/icons/SecurityScanOutlined';
import RadarChartOutlined from '@ant-design/icons/RadarChartOutlined';
import FundOutlined from '@ant-design/icons/FundOutlined';
import type { NavItemConfig } from 'layout/Dashboard/Drawer/DrawerContent/Navigation/NavItem';

interface NavGroup {
  id: string;
  title: string;
  type: 'group';
  children: NavItemConfig[];
}

const overview: NavGroup = {
  id: 'group-overview',
  title: 'Overview',
  type: 'group',
  children: [
    {
      id: 'dashboard',
      title: 'Dashboard',
      type: 'item',
      url: '/dashboard',
      icon: DashboardOutlined,
      breadcrumbs: false
    }
  ]
};

const management: NavGroup = {
  id: 'group-management',
  title: 'Management',
  type: 'group',
  children: [
    {
      id: 'keys',
      title: 'API Keys',
      type: 'item',
      url: '/keys',
      icon: KeyOutlined
    },
    {
      id: 'models',
      title: 'Models',
      type: 'item',
      url: '/models',
      icon: CloudServerOutlined
    },
    {
      id: 'plugins',
      title: 'Plugins',
      type: 'item',
      url: '/plugins',
      icon: ApiOutlined
    },
    {
      id: 'prompts',
      title: 'Prompt Management',
      type: 'item',
      url: '/prompts',
      icon: FileTextOutlined
    }
  ]
};

const monitoring: NavGroup = {
  id: 'group-monitoring',
  title: 'Monitoring',
  type: 'group',
  children: [
    {
      id: 'usage',
      title: 'Usage',
      type: 'item',
      url: '/usage',
      icon: BarChartOutlined
    },
    {
      id: 'costs',
      title: 'Cost Control',
      type: 'item',
      url: '/costs',
      icon: DollarOutlined
    },
    {
      id: 'finops',
      title: 'FinOps',
      type: 'item',
      url: '/finops',
      icon: FundOutlined
    },
    {
      id: 'audit',
      title: 'Audit Log',
      type: 'item',
      url: '/audit',
      icon: AuditOutlined
    }
  ]
};

const advanced: NavGroup = {
  id: 'group-advanced',
  title: 'Advanced',
  type: 'group',
  children: [
    {
      id: 'ab-testing',
      title: 'A/B Testing',
      type: 'item',
      url: '/ab-testing',
      icon: ExperimentOutlined
    },
    {
      id: 'sessions',
      title: 'Sessions',
      type: 'item',
      url: '/sessions',
      icon: MessageOutlined
    },
    {
      id: 'webhooks',
      title: 'Webhooks',
      type: 'item',
      url: '/webhooks',
      icon: BellOutlined
    }
  ]
};

const security: NavGroup = {
  id: 'group-security',
  title: 'Security Pipeline',
  type: 'group',
  children: [
    {
      id: 'guardrails',
      title: 'GuardRails',
      type: 'item',
      url: '/guardrails',
      icon: SafetyOutlined
    },
    {
      id: 'discovery',
      title: 'Data Discovery',
      type: 'item',
      url: '/discovery',
      icon: RadarChartOutlined
    },
    {
      id: 'dlp',
      title: 'DLP',
      type: 'item',
      url: '/dlp',
      icon: SecurityScanOutlined
    }
  ]
};

const system: NavGroup = {
  id: 'group-system',
  title: 'System',
  type: 'group',
  children: [
    {
      id: 'security',
      title: 'Posture & Compliance',
      type: 'item',
      url: '/security',
      icon: AuditOutlined
    },
    {
      id: 'settings',
      title: 'Settings',
      type: 'item',
      url: '/settings',
      icon: SettingOutlined
    }
  ]
};

export { overview, management, monitoring, advanced, security, system };
