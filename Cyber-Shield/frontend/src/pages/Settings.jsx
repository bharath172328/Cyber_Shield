import { useState } from 'react';
import { KeyRound, LogOut, Server, Settings as SettingsIcon, UserCircle } from 'lucide-react';
import { useNavigate } from 'react-router-dom';
import Badge from '../components/Badge';
import Button from '../components/Button';
import Input from '../components/Input';
import PageHeader from '../components/PageHeader';
import { useAuth } from '../context/AuthContext';
import { useToast } from '../context/ToastContext';
import { useAsync } from '../hooks/useAsync';
import api, { errorMessage } from '../services/api';
import { authService } from '../services/authService';
import { dashboardService } from '../services/dashboardService';
import { formatDate } from '../utils/format';

export default function Settings() {
  const { user, logout } = useAuth();
  const toast = useToast();
  const navigate = useNavigate();
  const [form, setForm] = useState({ current: '', next: '', confirm: '' });
  const [errors, setErrors] = useState({});
  const [saving, setSaving] = useState(false);
  const { data: profile } = useAsync(authService.me, { immediate: true });
  const { data: health, error: healthError, run: checkHealth, loading: checking } = useAsync(dashboardService.health, { immediate: true });

  const submit = async (e) => {
    e.preventDefault();
    const er = {};
    if (!form.current) er.current = 'Enter your current password';
    if (form.next.length < 8) er.next = 'Use at least 8 characters';
    if (form.confirm !== form.next) er.confirm = 'Passwords do not match';
    setErrors(er);
    if (Object.keys(er).length) return;
    setSaving(true);
    try { await authService.changePassword(form.current, form.next); toast.success('Password updated'); setForm({ current: '', next: '', confirm: '' }); }
    catch (err) { toast.error(errorMessage(err)); } finally { setSaving(false); }
  };

  return (
    <>
      <PageHeader icon={SettingsIcon} title="Settings" description="Manage your account and check the connection to the Cyber Shield engine." />
      <div className="grid gap-6 lg:grid-cols-2">
        <section className="glass p-6">
          <h3 className="mb-4 flex items-center gap-2 font-semibold text-white"><UserCircle className="h-5 w-5 text-brand-400" />Profile</h3>
          <dl className="space-y-3 text-sm">
            <div className="flex justify-between"><dt className="text-slate-500">Username</dt><dd className="text-slate-200">{user.username}</dd></div>
            <div className="flex justify-between"><dt className="text-slate-500">Role</dt><dd><Badge tone={user.role === 'admin' ? 'info' : 'neutral'}>{user.role}</Badge></dd></div>
            <div className="flex justify-between"><dt className="text-slate-500">Email</dt><dd className="text-slate-200">{profile?.email || '—'}</dd></div>
            <div className="flex justify-between"><dt className="text-slate-500">Member since</dt><dd className="text-slate-200">{formatDate(profile?.createdAt)}</dd></div>
            <div className="flex justify-between"><dt className="text-slate-500">Last sign-in</dt><dd className="text-slate-200">{formatDate(profile?.lastLogin)}</dd></div>
          </dl>
          <Button variant="secondary" icon={LogOut} className="mt-6" onClick={async () => { await logout(); navigate('/login'); }}>Sign out</Button>
        </section>

        <section className="glass p-6">
          <h3 className="mb-4 flex items-center gap-2 font-semibold text-white"><KeyRound className="h-5 w-5 text-brand-400" />Change password</h3>
          <form onSubmit={submit} className="space-y-4" noValidate>
            <Input label="Current password" type="password" autoComplete="current-password" value={form.current} error={errors.current} onChange={(e) => setForm({ ...form, current: e.target.value })} />
            <Input label="New password" type="password" autoComplete="new-password" value={form.next} error={errors.next} onChange={(e) => setForm({ ...form, next: e.target.value })} />
            <Input label="Confirm new password" type="password" autoComplete="new-password" value={form.confirm} error={errors.confirm} onChange={(e) => setForm({ ...form, confirm: e.target.value })} />
            <Button type="submit" loading={saving}>Update password</Button>
          </form>
        </section>

        <section className="glass p-6 lg:col-span-2">
          <h3 className="mb-4 flex items-center gap-2 font-semibold text-white"><Server className="h-5 w-5 text-brand-400" />Backend connection</h3>
          <div className="flex flex-wrap items-center justify-between gap-4 text-sm">
            <div className="space-y-1">
              <p className="text-slate-500">API endpoint</p><code className="font-mono text-slate-200">{api.defaults.baseURL}</code>
              <p className="pt-2 text-slate-500">Status</p>
              {health ? <p className="text-slate-200"><Badge tone="success">online</Badge> <span className="ml-2 text-xs text-slate-500">{health.database?.engine} · v{health.version}</span></p>
                : healthError ? <Badge tone="critical">unreachable</Badge> : <span className="text-slate-500">Checking…</span>}
            </div>
            <Button variant="secondary" loading={checking} onClick={() => checkHealth().catch(() => {})}>Re-check</Button>
          </div>
        </section>
      </div>
    </>
  );
}
