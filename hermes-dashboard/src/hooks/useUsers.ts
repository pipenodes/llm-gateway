import useSWR, { mutate as globalMutate } from 'swr';
import { fetcher, apiFetch } from 'lib/fetcher';
import type { UserRecord } from 'mocks/db';

export type { UserRecord };

interface UsersResponse {
  data: UserRecord[];
  total: number;
}

export function useUsers(params?: { search?: string; role?: string; status?: string }) {
  const query = new URLSearchParams();
  if (params?.search) query.set('search', params.search);
  if (params?.role) query.set('role', params.role);
  if (params?.status) query.set('status', params.status);
  const key = `/api/users?${query.toString()}`;

  const { data, error, isLoading, mutate } = useSWR<UsersResponse>(key, fetcher);

  return {
    users: data?.data ?? [],
    total: data?.total ?? 0,
    isLoading,
    error,
    mutate,
    refetch: () => globalMutate((k: string) => typeof k === 'string' && k.startsWith('/api/users'))
  };
}

export async function createUser(body: Omit<UserRecord, 'id' | 'createdAt' | 'lastLogin' | 'avatarUrl'>) {
  return apiFetch<UserRecord>('/api/users', { method: 'POST', body });
}

export async function updateUser(id: string, body: Partial<UserRecord>) {
  return apiFetch<UserRecord>(`/api/users/${id}`, { method: 'PUT', body });
}

export async function deleteUser(id: string) {
  return apiFetch<void>(`/api/users/${id}`, { method: 'DELETE' });
}
