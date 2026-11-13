import { motion } from 'framer-motion';
import Skeleton from './Skeleton';

const tones = { brand: 'text-brand-400 bg-brand-500/10', success: 'text-emerald-400 bg-emerald-500/10', warning: 'text-amber-400 bg-amber-500/10', danger: 'text-red-400 bg-red-500/10', violet: 'text-violet-400 bg-violet-500/10' };

export default function DashboardCard({ title, value, hint, icon: Icon, tone = 'brand', loading, index = 0 }) {
  return (
    <motion.div
      initial={{ opacity: 0, y: 14 }} animate={{ opacity: 1, y: 0 }} transition={{ delay: index * 0.06, duration: 0.35 }}
      whileHover={{ y: -3 }} className="glass p-5"
    >
      <div className="flex items-start justify-between">
        <p className="text-xs font-medium uppercase tracking-wide text-slate-400">{title}</p>
        {Icon && <span className={`rounded-xl p-2 ${tones[tone]}`}><Icon className="h-4 w-4" /></span>}
      </div>
      {loading ? <Skeleton className="mt-3 h-8 w-24" /> : <p className="mt-2 text-3xl font-semibold tracking-tight text-white">{value}</p>}
      {hint && (loading ? <Skeleton className="mt-2 h-3 w-32" /> : <p className="mt-1 text-xs text-slate-500">{hint}</p>)}
    </motion.div>
  );
}
