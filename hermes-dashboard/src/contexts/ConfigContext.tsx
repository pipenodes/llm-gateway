import { createContext, useMemo } from 'react';
import type { ReactNode } from 'react';

import config from 'config';
import { useLocalStorage } from 'hooks/useLocalStorage';

interface AppConfig {
  fontFamily: string;
}

interface ConfigContextValue {
  state: AppConfig;
  setState: (s: AppConfig) => void;
  setField: <K extends keyof AppConfig>(key: K, value: AppConfig[K]) => void;
  resetState: () => void;
}

export const ConfigContext = createContext<ConfigContextValue | undefined>(undefined);

export function ConfigProvider({ children }: { children: ReactNode }) {
  const { state, setState, setField, resetState } = useLocalStorage<AppConfig>('app-config', config);

  const value = useMemo(
    () => ({ state, setState, setField, resetState }),
    [state, setState, setField, resetState]
  );

  return <ConfigContext.Provider value={value}>{children}</ConfigContext.Provider>;
}
