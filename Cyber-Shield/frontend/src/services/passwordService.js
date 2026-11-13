import api from './api';

export const passwordService = {
  analyze: (password) => api.post('/password/analyze', { password }).then((r) => r.data),
};
