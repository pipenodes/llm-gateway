import { userHandlers } from './users';
import { notificationHandlers } from './notifications';

export const handlers = [...userHandlers, ...notificationHandlers];
