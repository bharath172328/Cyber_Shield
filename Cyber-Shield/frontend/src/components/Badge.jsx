const tones = {
  neutral: 'bg-white/5 text-slate-300 border-white/10',
  info: 'bg-sky-500/10 text-sky-300 border-sky-500/20',
  success: 'bg-emerald-500/10 text-emerald-300 border-emerald-500/20',
  warning: 'bg-amber-500/10 text-amber-300 border-amber-500/20',
  danger: 'bg-orange-500/10 text-orange-300 border-orange-500/20',
  critical: 'bg-red-500/10 text-red-300 border-red-500/25',
};

export default function Badge({ tone = 'neutral', children, className = '' }) {
  return <span className={`inline-flex items-center gap-1 rounded-full border px-2.5 py-0.5 text-xs font-medium whitespace-nowrap ${tones[tone] || tones.neutral} ${className}`}>{children}</span>;
}
