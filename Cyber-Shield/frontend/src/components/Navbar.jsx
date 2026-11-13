import { useEffect, useRef, useState } from 'react';
import { AnimatePresence, motion } from 'framer-motion';
import { Bell, LogOut, Menu, Settings as SettingsIcon } from 'lucide-react';
import { useNavigate } from 'react-router-dom';
import { useAuth } from '../context/AuthContext';
import { dashboardService } from '../services/dashboardService';
import { eventLabel, timeAgo } from '../utils/format';
import { severityTone } from '../utils/tone';
import Badge from './Badge';
import Breadcrumbs from './Breadcrumbs';
import Tooltip from './Tooltip';

function useOutsideClose(ref, close) {
  useEffect(() => {
    const h = (e) => ref.current && !ref.current.contains(e.target) && close();
    document.addEventListener('mousedown', h);
    return () => document.removeEventListener('mousedown', h);
  }, [ref, close]);
}

export default function Navbar({ onMenu }) {
  const { user, logout } = useAuth();
  const navigate = useNavigate();
  const [panel, setPanel] = useState(null); // 'bell' | 'user' | null
  const [events, setEvents] = useState(null);
  const ref = useRef(null);
  useOutsideClose(ref, () => setPanel(null));

  const toggleBell = async () => {
    const next = panel === 'bell' ? null : 'bell';
    setPanel(next);
    if (next) {
      try { setEvents((await dashboardService.get()).recentEvents.slice(0, 6)); } catch { setEvents([]); }
    }
  };

  return (
    <header className="sticky top-0 z-20 flex items-center gap-3 border-b border-white/10 bg-ink-950/70 px-4 py-3 backdrop-blur-xl sm:px-6">
      <button onClick={onMenu} aria-label="Open menu" className="rounded-lg p-2 text-slate-300 hover:bg-white/10 lg:hidden"><Menu className="h-5 w-5" /></button>
      <div className="hidden sm:block"><Breadcrumbs /></div>
      <div className="flex-1" />
      <div ref={ref} className="relative flex items-center gap-1">
        <Tooltip text="Recent activity">
          <button onClick={toggleBell} aria-label="Notifications" className="relative rounded-xl p-2 text-slate-300 hover:bg-white/10"><Bell className="h-5 w-5" /></button>
        </Tooltip>
        <button onClick={() => setPanel(panel === 'user' ? null : 'user')} className="flex items-center gap-2.5 rounded-xl py-1.5 pl-2 pr-3 hover:bg-white/10" aria-label="User menu">
          <span className="flex h-8 w-8 items-center justify-center rounded-full bg-gradient-to-br from-brand-400 to-indigo-500 text-sm font-semibold text-ink-950">{user?.username?.[0]?.toUpperCase()}</span>
          <span className="hidden text-left leading-tight sm:block">
            <span className="block text-sm font-medium text-slate-100">{user?.username}</span>
            <span className="block text-[11px] capitalize text-slate-500">{user?.role}</span>
          </span>
        </button>

        <AnimatePresence>
          {panel && (
            <motion.div initial={{ opacity: 0, y: -6, scale: 0.98 }} animate={{ opacity: 1, y: 0, scale: 1 }} exit={{ opacity: 0, y: -6 }}
              className="absolute right-0 top-full mt-2 w-80 max-w-[90vw] overflow-hidden rounded-2xl border border-white/10 bg-ink-800 shadow-2xl">
              {panel === 'bell' ? (
                <div>
                  <p className="border-b border-white/10 px-4 py-3 text-sm font-semibold text-white">Recent activity</p>
                  {events === null && <p className="px-4 py-6 text-center text-sm text-slate-500">Loading…</p>}
                  {events?.length === 0 && <p className="px-4 py-6 text-center text-sm text-slate-500">No recent events</p>}
                  {events?.map((e) => (
                    <div key={e.id} className="border-b border-white/5 px-4 py-2.5 last:border-0">
                      <div className="flex items-center justify-between gap-2"><Badge tone={severityTone[e.severity]}>{eventLabel(e.event_type)}</Badge><span className="text-[11px] text-slate-500">{timeAgo(e.created_at)}</span></div>
                      <p className="mt-1 line-clamp-2 text-xs text-slate-400">{e.description}</p>
                    </div>
                  ))}
                  <button onClick={() => { setPanel(null); navigate('/security-logs'); }} className="w-full px-4 py-2.5 text-center text-xs font-medium text-brand-400 hover:bg-white/5">View all logs</button>
                </div>
              ) : (
                <div className="p-1.5">
                  <button onClick={() => { setPanel(null); navigate('/settings'); }} className="flex w-full items-center gap-2.5 rounded-xl px-3 py-2.5 text-sm text-slate-200 hover:bg-white/5"><SettingsIcon className="h-4 w-4" />Settings</button>
                  <button onClick={async () => { setPanel(null); await logout(); navigate('/login'); }} className="flex w-full items-center gap-2.5 rounded-xl px-3 py-2.5 text-sm text-red-300 hover:bg-red-500/10"><LogOut className="h-4 w-4" />Sign out</button>
                </div>
              )}
            </motion.div>
          )}
        </AnimatePresence>
      </div>
    </header>
  );
}
