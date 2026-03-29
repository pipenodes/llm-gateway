import { http, HttpResponse, delay } from 'msw';
import { db } from '../db';

const LAT = 400; // latência simulada em ms

export const userHandlers = [
  // GET /api/users — lista com busca e filtro
  http.get('/api/users', async ({ request }) => {
    await delay(LAT);
    const url = new URL(request.url);
    const search = url.searchParams.get('search')?.toLowerCase() ?? '';
    const role = url.searchParams.get('role') ?? '';
    const status = url.searchParams.get('status') ?? '';

    let data = db.getUsers();
    if (search) data = data.filter((u) => u.name.toLowerCase().includes(search) || u.email.toLowerCase().includes(search));
    if (role) data = data.filter((u) => u.role === role);
    if (status) data = data.filter((u) => u.status === status);

    return HttpResponse.json({ data, total: data.length });
  }),

  // GET /api/users/:id
  http.get('/api/users/:id', async ({ params }) => {
    await delay(LAT);
    const user = db.getUserById(params.id as string);
    if (!user) return HttpResponse.json({ message: 'Usuário não encontrado' }, { status: 404 });
    return HttpResponse.json(user);
  }),

  // POST /api/users
  http.post('/api/users', async ({ request }) => {
    await delay(LAT);
    const body = await request.json() as Parameters<typeof db.createUser>[0];
    const user = db.createUser(body);
    return HttpResponse.json(user, { status: 201 });
  }),

  // PUT /api/users/:id
  http.put('/api/users/:id', async ({ params, request }) => {
    await delay(LAT);
    const body = await request.json() as Partial<Parameters<typeof db.createUser>[0]>;
    const updated = db.updateUser(params.id as string, body);
    if (!updated) return HttpResponse.json({ message: 'Usuário não encontrado' }, { status: 404 });
    return HttpResponse.json(updated);
  }),

  // DELETE /api/users/:id
  http.delete('/api/users/:id', async ({ params }) => {
    await delay(LAT);
    const ok = db.deleteUser(params.id as string);
    if (!ok) return HttpResponse.json({ message: 'Usuário não encontrado' }, { status: 404 });
    return new HttpResponse(null, { status: 204 });
  }),
];
