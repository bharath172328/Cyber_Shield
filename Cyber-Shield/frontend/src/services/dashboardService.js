import api from './api';

export const dashboardService = {
  get: (scope) => api.get('/dashboard', { params: scope ? { scope } : {} }).then((r) => r.data),
  logs: (params) => api.get('/logs', { params }).then((r) => r.data),
  reports: () => api.get('/reports').then((r) => r.data.items),
  generateReport: (title) => api.post('/reports/generate', { title }).then((r) => r.data),
  users: () => api.get('/admin/users').then((r) => r.data.items),
  health: () => api.get('/health').then((r) => r.data),
};
