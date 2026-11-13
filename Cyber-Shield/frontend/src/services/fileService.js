import api from './api';

const upload = (path, file, extra = {}) => {
  const form = new FormData();
  form.append('file', file);
  Object.entries(extra).forEach(([k, v]) => v && form.append(k, v));
  return api.post(path, form, { timeout: 120000 }).then((r) => r.data);
};

export const fileService = {
  hash: (file) => upload('/file/hash', file),
  analyze: (file) => upload('/file/analyze', file),
  listBaselines: () => api.get('/file/integrity').then((r) => r.data.items),
  createBaseline: (file) => upload('/file/integrity/baseline', file),
  verify: (file, expectedHash) => upload('/file/integrity/verify', file, { expectedHash }),
  deleteBaseline: (id) => api.delete(`/file/integrity/${id}`).then((r) => r.data),
};
