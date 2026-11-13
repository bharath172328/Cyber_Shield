import { useState } from 'react';
import { motion } from 'framer-motion';
import { Check, Lock, Mail, User } from 'lucide-react';
import { Link, useNavigate } from 'react-router-dom';
import Button from '../components/Button';
import Input from '../components/Input';
import { useAuth } from '../context/AuthContext';
import { useToast } from '../context/ToastContext';
import { errorMessage } from '../services/api';
import { authService } from '../services/authService';

const rules = [
  { label: '8+ characters', test: (p) => p.length >= 8 },
  { label: 'Upper & lower case', test: (p) => /[a-z]/.test(p) && /[A-Z]/.test(p) },
  { label: 'A number', test: (p) => /\d/.test(p) },
  { label: 'A symbol', test: (p) => /[^A-Za-z0-9]/.test(p) },
];

export default function Register() {
  const { login } = useAuth();
  const toast = useToast();
  const navigate = useNavigate();
  const [form, setForm] = useState({ username: '', email: '', password: '', confirm: '' });
  const [errors, setErrors] = useState({});
  const [loading, setLoading] = useState(false);
  const set = (k) => (e) => setForm({ ...form, [k]: e.target.value });

  const validate = () => {
    const e = {};
    if (!/^[A-Za-z0-9_.-]{3,32}$/.test(form.username)) e.username = '3-32 characters: letters, numbers, . _ -';
    if (form.email && !/^\S+@\S+\.\S+$/.test(form.email)) e.email = 'Enter a valid email address';
    if (form.password.length < 8) e.password = 'Use at least 8 characters';
    if (form.confirm !== form.password) e.confirm = 'Passwords do not match';
    setErrors(e);
    return Object.keys(e).length === 0;
  };

  const submit = async (ev) => {
    ev.preventDefault();
    if (!validate()) return;
    setLoading(true);
    try {
      await authService.register(form.username, form.email, form.password);
      await login(form.username, form.password);
      toast.success('Account created');
      navigate('/dashboard', { replace: true });
    } catch (err) { toast.error(errorMessage(err)); } finally { setLoading(false); }
  };

  return (
    <motion.div initial={{ opacity: 0, y: 16 }} animate={{ opacity: 1, y: 0 }} className="glass p-8">
      <h1 className="text-2xl font-semibold text-white">Create account</h1>
      <p className="mt-1 text-sm text-slate-400">The first account registered becomes the administrator.</p>
      <form onSubmit={submit} className="mt-6 space-y-4" noValidate>
        <Input label="Username" icon={User} autoComplete="username" value={form.username} error={errors.username} onChange={set('username')} />
        <Input label="Email (optional)" icon={Mail} type="email" autoComplete="email" value={form.email} error={errors.email} onChange={set('email')} />
        <Input label="Password" icon={Lock} type="password" autoComplete="new-password" value={form.password} error={errors.password} onChange={set('password')} />
        <div className="grid grid-cols-2 gap-1.5">
          {rules.map((r) => { const ok = r.test(form.password); return (
            <span key={r.label} className={`flex items-center gap-1.5 text-xs ${ok ? 'text-emerald-400' : 'text-slate-500'}`}><Check className={`h-3.5 w-3.5 ${ok ? '' : 'opacity-30'}`} />{r.label}</span>
          ); })}
        </div>
        <Input label="Confirm password" icon={Lock} type="password" autoComplete="new-password" value={form.confirm} error={errors.confirm} onChange={set('confirm')} />
        <Button type="submit" loading={loading} className="w-full">Create account</Button>
      </form>
      <p className="mt-6 text-center text-sm text-slate-400">Already registered? <Link to="/login" className="font-medium text-brand-400 hover:underline">Sign in</Link></p>
    </motion.div>
  );
}
