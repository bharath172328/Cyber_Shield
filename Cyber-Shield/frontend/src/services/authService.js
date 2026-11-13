import api from './api';

export const authService = {
  login: (username, password) => api.post('/auth/login', { username, password }).then((r) => r.data),
  register: (username, email, password) => api.post('/auth/register', { username, email, password }).then((r) => r.data),
  me: () => api.get('/auth/me').then((r) => r.data.user),
  logout: () => api.post('/auth/logout').then((r) => r.data),
  changePassword: (currentPassword, newPassword) => api.post('/auth/change-password', { currentPassword, newPassword }).then((r) => r.data),
};
