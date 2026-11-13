import { forwardRef, useId } from 'react';

const Input = forwardRef(function Input({ label, error, hint, icon: Icon, right, className = '', id, ...props }, ref) {
  const auto = useId();
  const inputId = id || auto;
  return (
    <div className={className}>
      {label && <label htmlFor={inputId} className="mb-1.5 block text-xs font-medium uppercase tracking-wide text-slate-400">{label}</label>}
      <div className="relative">
        {Icon && <Icon className="pointer-events-none absolute left-3.5 top-1/2 h-4 w-4 -translate-y-1/2 text-slate-500" />}
        <input
          id={inputId}
          ref={ref}
          aria-invalid={!!error}
          aria-describedby={error ? `${inputId}-err` : undefined}
          className={`field ${Icon ? 'pl-10' : ''} ${right ? 'pr-11' : ''} ${error ? '!border-red-500/60 focus:!ring-red-500/40' : ''}`}
          {...props}
        />
        {right && <div className="absolute right-2 top-1/2 -translate-y-1/2">{right}</div>}
      </div>
      {error ? <p id={`${inputId}-err`} className="mt-1.5 text-xs text-red-400">{error}</p> : hint && <p className="mt-1.5 text-xs text-slate-500">{hint}</p>}
    </div>
  );
});
export default Input;
