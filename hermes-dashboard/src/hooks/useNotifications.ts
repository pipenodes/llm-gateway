import useSWR, { mutate } from 'swr';
import { fetcher, apiFetch } from 'lib/fetcher';
import type { NotificationRecord } from 'mocks/db';

interface NotificationsResponse {
  data: NotificationRecord[];
  unreadCount: number;
}

const KEY = '/api/notifications';

export function useNotifications() {
  const { data, error, isLoading } = useSWR<NotificationsResponse>(KEY, fetcher, {
    refreshInterval: 30_000 // polling a cada 30s
  });

  const markAllRead = async () => {
    await apiFetch('/api/notifications/read-all', { method: 'POST' });
    mutate(KEY);
  };

  const markRead = async (id: string) => {
    await apiFetch(`/api/notifications/${id}/read`, { method: 'POST' });
    mutate(KEY);
  };

  return {
    notifications: data?.data ?? [],
    unreadCount: data?.unreadCount ?? 0,
    isLoading,
    error: !!error,
    markAllRead,
    markRead,
    refresh: () => mutate(KEY)
  };
}
