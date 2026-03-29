import { use } from 'react';
import { ConfigContext } from 'contexts/ConfigContext';

export default function useConfig() {
  const context = use(ConfigContext);
  if (!context) throw new Error('useConfig must be used inside ConfigProvider');
  return context;
}
