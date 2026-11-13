import { AnimatePresence, motion } from 'framer-motion';
import { ShieldCheck, X } from 'lucide-react';
import { NavLink } from 'react-router-dom';
import { useAuth } from '../context/AuthContext';
import { NAV_ITEMS } from './navItems';

function NavList({ onNavigate }) {
  const { isAdmin } = useAuth();
  return (
    <nav className="flex-1 space-y-1 overflow-y-auto px-3 py-4">
      {NAV_ITEMS.filter((n) => !n.adminOnly || isAdmin).map(({ to, label, icon: Icon }) => (
        <NavLink key={to} to={to} onClick={onNavigate}
          className={({ isActive }) => `group relative flex items-center gap-3 rounded-xl px-3 py-2.5 text-sm transition-colors ${isActive ? 'bg-brand-500/10 text-white' : 'text-slate-400 hover:bg-white/5 hover:text-slate-100'}`}>
          {({ isActive }) => (
            <>
              {isActive && <motion.span layoutId="nav-active" className="absolute inset-y-1.5 left-0 w-1 rounded-full bg-brand-400" />}
              <Icon className={`h-[18px] w-[18px] ${isActive ? 'text-brand-400' : ''}`} />
              {label}
            </>
          )}
        </NavLink>
      ))}
    </nav>
  );
}

function Brand() {
  return (
    <div className="flex items-center gap-2.5 px-5 py-5">
      <span className="rounded-xl bg-brand-500/15 p-2 text-brand-400"><ShieldCheck className="h-5 w-5" /></span>
      <div className="leading-tight">
        <p className="font-semibold tracking-wide text-white">CYBER SHIELD</p>
        <p className="text-[10px] uppercase tracking-widest text-slate-500">Threat Monitoring</p>
      </div>
    </div>
  );
}

export default function Sidebar({ mobileOpen, onClose }) {
  return (
    <>
      <aside className="fixed inset-y-0 left-0 z-30 hidden w-64 flex-col border-r border-white/10 bg-ink-900/70 backdrop-blur-xl lg:flex">
        <Brand />
        <NavList />
      </aside>

      <AnimatePresence>
        {mobileOpen && (
          <div className="fixed inset-0 z-50 lg:hidden">
            <motion.div className="absolute inset-0 bg-black/60" initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} onClick={onClose} />
            <motion.aside
              className="absolute inset-y-0 left-0 flex w-72 max-w-[85%] flex-col border-r border-white/10 bg-ink-900"
              initial={{ x: '-100%' }} animate={{ x: 0 }} exit={{ x: '-100%' }} transition={{ type: 'spring', stiffness: 340, damping: 34 }}
            >
              <button onClick={onClose} aria-label="Close menu" className="absolute right-3 top-4 rounded-lg p-1.5 text-slate-400 hover:bg-white/10"><X className="h-5 w-5" /></button>
              <Brand />
              <NavList onNavigate={onClose} />
            </motion.aside>
          </div>
        )}
      </AnimatePresence>
    </>
  );
}
