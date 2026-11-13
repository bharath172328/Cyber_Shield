import { AnimatePresence, motion } from 'framer-motion';
import { AlertOctagon, AlertTriangle, CheckCircle2, Info, X } from 'lucide-react';

const styles = {
  success: { icon: CheckCircle2, cls: 'text-emerald-300 border-emerald-500/30' },
  error: { icon: AlertOctagon, cls: 'text-red-300 border-red-500/30' },
  warning: { icon: AlertTriangle, cls: 'text-amber-300 border-amber-500/30' },
  info: { icon: Info, cls: 'text-sky-300 border-sky-500/30' },
};

export default function Notification({ toasts, onDismiss }) {
  return (
    <div aria-live="polite" className="pointer-events-none fixed right-4 top-4 z-[60] flex w-[calc(100%-2rem)] max-w-sm flex-col gap-2">
      <AnimatePresence>
        {toasts.map((t) => {
          const { icon: Icon, cls } = styles[t.type] || styles.info;
          return (
            <motion.div key={t.id} layout initial={{ opacity: 0, x: 40, scale: 0.95 }} animate={{ opacity: 1, x: 0, scale: 1 }} exit={{ opacity: 0, x: 40 }}
              className={`pointer-events-auto flex items-start gap-3 rounded-xl border bg-ink-800/95 p-3.5 shadow-xl backdrop-blur ${cls}`}>
              <Icon className="mt-0.5 h-5 w-5 shrink-0" />
              <p className="flex-1 text-sm text-slate-100">{t.message}</p>
              <button onClick={() => onDismiss(t.id)} aria-label="Dismiss" className="text-slate-400 hover:text-white"><X className="h-4 w-4" /></button>
            </motion.div>
          );
        })}
      </AnimatePresence>
    </div>
  );
}
