import useSWR, { mutate } from 'swr';
import { useMemo } from 'react';

interface MenuMaster {
  isDashboardDrawerOpened: boolean;
}

const initialState: MenuMaster = {
  isDashboardDrawerOpened: false
};

const endpoints = {
  key: 'api/menu',
  master: 'master'
};

export function useGetMenuMaster() {
  const { data, isLoading } = useSWR<MenuMaster>(
    endpoints.key + endpoints.master,
    () => initialState,
    {
      revalidateIfStale: false,
      revalidateOnFocus: false,
      revalidateOnReconnect: false
    }
  );

  return useMemo(
    () => ({
      menuMaster: data ?? initialState,
      menuMasterLoading: isLoading
    }),
    [data, isLoading]
  );
}

export function handlerDrawerOpen(isDashboardDrawerOpened: boolean) {
  mutate(
    endpoints.key + endpoints.master,
    (currentMenuMaster: MenuMaster | undefined) => ({
      ...(currentMenuMaster ?? initialState),
      isDashboardDrawerOpened
    }),
    false
  );
}
