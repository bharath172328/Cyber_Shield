import axios from 'axios';

export const TOKEN_KEY = 'cs_token';

const api = axios.create({
  baseURL: import.meta.env.VITE_API_URL || '/api',
  timeout: 30000,
});

api.interceptors.request.use((config) => {
  const token = localStorage.getItem(TOKEN_KEY);
  if (token) config.headers.Authorization = `Bearer ${token}`;
  return config;
});

api.interceptors.response.use(
  (res) => res,
  (err) => {
    const isLogin = err.config?.url?.includes('/auth/login');
    if (err.response?.status === 401 && !isLogin) window.dispatchEvent(new Event('cs:unauthorized'));
    return Promise.reject(err);
  },
);

/** Turn any axios error into a human readable string. */
export function errorMessage(err) {
  if (err?.response?.data?.error) return err.response.data.error;
  if (err?.code === 'ERR_NETWORK') return 'Cannot reach the Cyber Shield server. Is the backend running?';
  if (err?.code === 'ECONNABORTED') return 'The request timed out. Please try again.';
  return err?.message || 'Something went wrong';
}

export default api;
