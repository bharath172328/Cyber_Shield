import { useState } from 'react';
import { Check, Copy } from 'lucide-react';
import Tooltip from './Tooltip';

export default function CopyButton({ value }) {
  const [done, setDone] = useState(false);
  const copy = async () => {
    try { await navigator.clipboard.writeText(value); setDone(true); setTimeout(() => setDone(false), 1500); } catch { /* clipboard unavailable */ }
  };
  return (
    <Tooltip text={done ? 'Copied!' : 'Copy'}>
      <button onClick={copy} aria-label="Copy to clipboard" className="rounded-lg p-1.5 text-slate-400 hover:bg-white/10 hover:text-white">
        {done ? <Check className="h-4 w-4 text-emerald-400" /> : <Copy className="h-4 w-4" />}
      </button>
    </Tooltip>
  );
}
