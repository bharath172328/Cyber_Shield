import { motion } from 'framer-motion';
import Skeleton from './Skeleton';

export default function ThreatCard({ title, icon: Icon, total, threats, unit = 'scanned', loading, index = 0 }) {
  const pct = total ? Math.round((threats / total) * 100) : 0;
  return (
    <motion.div initial={{ opacity: 0, y: 14 }} animate={{ opacity: 1, y: 0 }} transition={{ delay: index * 0.06 }} className="glass p-5">
      <div className="flex items-center gap-3">
        <span className="rounded-xl bg-white/5 p-2 text-brand-400"><Icon className="h-4 w-4" /></span>
        <p className="text-sm font-medium text-slate-200">{title}</p>
      </div>
      {loading ? <Skeleton className="mt-4 h-10 w-full" /> : (
        <>
          <div className="mt-4 flex items-end justify-between">
            <p className="text-2xl font-semibold text-white">{threats}<span className="ml-1 text-sm font-normal text-slate-500">flagged</span></p>
            <p className="text-xs text-slate-500">{total} {unit}</p>
          </div>
          <div className="mt-3 h-1.5 overflow-hidden rounded-full bg-white/10">
            <motion.div className={`h-full rounded-full ${pct > 40 ? 'bg-red-400' : pct > 0 ? 'bg-amber-400' : 'bg-emerald-400'}`} initial={{ width: 0 }} animate={{ width: `${pct}%` }} transition={{ duration: 0.8 }} />
          </div>
        </>
      )}
    </motion.div>
  );
}
