import api from './api';

export const networkService = {
  scan: ({ host, startPort, endPort }) =>
    api.post('/network/scan', { host, startPort, endPort, authorized: true }, { timeout: 60000 }).then((r) => r.data),
};
