import api from './api';

export const urlService = {
  analyze: (url) => api.post('/url/analyze', { url }).then((r) => r.data),
};
