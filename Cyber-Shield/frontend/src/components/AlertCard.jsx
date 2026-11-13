import { AlertOctagon, AlertTriangle, CheckCircle2, Info } from 'lucide-react';

const map = {
  info: { icon: Info, cls: 'border-sky-500/20 bg-sky-500/5 text-sky-300' },
  success: { icon: CheckCircle2, cls: 'border-emerald-500/20 bg-emerald-500/5 text-emerald-300' },
  warning: { icon: AlertTriangle, cls: 'border-amber-500/20 bg-amber-500/5 text-amber-300' },
  critical: { icon: AlertOctagon, cls: 'border-red-500/25 bg-red-500/5 text-red-300' },
};

export default function AlertCard({ tone = 'info', title, children, className = '' }) {
  const { icon: Icon, cls } = map[tone] || map.info;
  return (
    <div role="alert" className={`flex gap-3 rounded-xl border p-4 ${cls} ${className}`}>
      <Icon className="mt-0.5 h-5 w-5 shrink-0" />
      <div className="text-sm">
        {title && <p className="font-semibold">{title}</p>}
        <div className={title ? 'mt-0.5 text-slate-300/90' : 'text-slate-200'}>{children}</div>
      </div>
    </div>
  );
}
