import { Loader2 } from 'lucide-react';

export default function LoadingSpinner({ label = 'Loading…', className = '' }) {
  return (
    <div role="status" className={`flex items-center justify-center gap-2 py-10 text-sm text-slate-400 ${className}`}>
      <Loader2 className="h-5 w-5 animate-spin text-brand-400" />
      <span>{label}</span>
    </div>
  );
}
