import { motion } from 'framer-motion';

export default function PageHeader({ title, description, icon: Icon, actions }) {
  return (
    <motion.div initial={{ opacity: 0, y: 8 }} animate={{ opacity: 1, y: 0 }} className="mb-6 flex flex-wrap items-start justify-between gap-4">
      <div className="flex items-start gap-3">
        {Icon && <span className="mt-0.5 rounded-xl border border-white/10 bg-white/5 p-2.5 text-brand-400"><Icon className="h-5 w-5" /></span>}
        <div>
          <h1 className="text-2xl font-semibold tracking-tight text-white">{title}</h1>
          {description && <p className="mt-1 max-w-2xl text-sm text-slate-400">{description}</p>}
        </div>
      </div>
      {actions}
    </motion.div>
  );
}
