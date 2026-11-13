import { createContext, useCallback, useContext, useEffect, useMemo, useState } from 'react';
import { authService } from '../services/authService';
import { TOKEN_KEY } from '../services/api';
import { useToast } from './ToastContext';

const AuthContext = createContext(null);

export function AuthProvider({ children }) {
  const toast = useToast();
  const [user, setUser] = useState(null);
  const [loading, setLoading] = useState(!!localStorage.getItem(TOKEN_KEY));

  const clear = useCallback(() => { localStorage.removeItem(TOKEN_KEY); setUser(null); }, []);

  useEffect(() => {
    if (!localStorage.getItem(TOKEN_KEY)) return;
    authService.me().then(setUser).catch(clear).finally(() => setLoading(false));
  }, [clear]);

  useEffect(() => {
    const onExpired = () => { if (localStorage.getItem(TOKEN_KEY)) { clear(); toast.warning('Your session has expired. Please sign in again.'); } };
    window.addEventListener('cs:unauthorized', onExpired);
    return () => window.removeEventListener('cs:unauthorized', onExpired);
  }, [clear, toast]);

  const login = useCallback(async (username, password) => {
    const res = await authService.login(username, password);
    localStorage.setItem(TOKEN_KEY, res.token);
    setUser(res.user);
    return res.user;
  }, []);

  const logout = useCallback(async () => {
    try { await authService.logout(); } catch { /* token may already be invalid */ }
    clear();
  }, [clear]);

  const value = useMemo(() => ({ user, loading, login, logout, isAdmin: user?.role === 'admin' }), [user, loading, login, logout]);
  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>;
}

export const useAuth = () => useContext(AuthContext);
