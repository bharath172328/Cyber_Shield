import { motion } from 'framer-motion';
import Skeleton from './Skeleton';

export default function ChartCard({ title, subtitle, children, loading, className = '', height = 260, action }) {
  return (
    <motion.section initial={{ opacity: 0, y: 14 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.4 }} className={`glass p-5 ${className}`}>
      <div className="mb-4 flex items-start justify-between gap-3">
        <div>
          <h3 className="font-semibold text-white">{title}</h3>
          {subtitle && <p className="text-xs text-slate-500">{subtitle}</p>}
        </div>
        {action}
      </div>
      <div style={{ height }} className="w-full">{loading ? <Skeleton className="h-full w-full" /> : children}</div>
    </motion.section>
  );
}
