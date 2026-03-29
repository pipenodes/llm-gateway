import { http, HttpResponse, delay } from 'msw';
import { db } from '../db';

export const notificationHandlers = [
  http.get('/api/notifications', async () => {
    await delay(300);
    return HttpResponse.json({
      data: db.getNotifications(),
      unreadCount: db.getUnreadCount()
    });
  }),

  http.post('/api/notifications/read-all', async () => {
    await delay(200);
    db.markAllRead();
    return HttpResponse.json({ ok: true });
  }),

  http.post('/api/notifications/:id/read', async ({ params }) => {
    await delay(150);
    db.markRead(params.id as string);
    return HttpResponse.json({ ok: true });
  }),
];
