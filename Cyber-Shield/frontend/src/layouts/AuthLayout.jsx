import { motion } from 'framer-motion';
import { Activity, Lock, ShieldCheck, Zap } from 'lucide-react';
import { Navigate, Outlet } from 'react-router-dom';
import { useAuth } from '../context/AuthContext';

const points = [
  { icon: Activity, text: 'Real-time threat analysis for passwords, URLs, files and networks' },
  { icon: Lock, text: 'C++ security engine with PBKDF2-hashed credentials' },
  { icon: Zap, text: 'Audit-ready logs, reports and security scoring' },
];

export default function AuthLayout() {
  const { user, loading } = useAuth();
  if (!loading && user) return <Navigate to="/dashboard" replace />;
  return (
    <div className="grid min-h-screen lg:grid-cols-2">
      <div className="relative hidden flex-col justify-between overflow-hidden border-r border-white/10 p-12 lg:flex">
        <div className="absolute inset-0 opacity-[0.07]" style={{ backgroundImage: 'linear-gradient(#38bdf8 1px, transparent 1px), linear-gradient(90deg, #38bdf8 1px, transparent 1px)', backgroundSize: '44px 44px' }} />
        <div className="relative flex items-center gap-3">
          <span className="rounded-xl bg-brand-500/15 p-2.5 text-brand-400"><ShieldCheck className="h-6 w-6" /></span>
          <span className="text-lg font-semibold tracking-wide text-white">CYBER SHIELD</span>
        </div>
        <div className="relative">
          <motion.h2 initial={{ opacity: 0, y: 16 }} animate={{ opacity: 1, y: 0 }} className="max-w-md text-4xl font-semibold leading-tight text-white">
            Detect, analyse and monitor threats from one console.
          </motion.h2>
          <ul className="mt-8 space-y-4">
            {points.map(({ icon: Icon, text }, i) => (
              <motion.li key={text} initial={{ opacity: 0, x: -14 }} animate={{ opacity: 1, x: 0 }} transition={{ delay: 0.2 + i * 0.1 }} className="flex items-start gap-3 text-slate-300">
                <span className="mt-0.5 rounded-lg bg-white/5 p-1.5 text-brand-400"><Icon className="h-4 w-4" /></span>{text}
              </motion.li>
            ))}
          </ul>
        </div>
        <p className="relative text-xs text-slate-500">MCA Academic Project · C++17 engine · React dashboard</p>
      </div>
      <div className="flex items-center justify-center p-6"><div className="w-full max-w-md"><Outlet /></div></div>
    </div>
  );
}
