import { useRef, useState } from 'react';
import { FileUp, X } from 'lucide-react';
import { formatBytes } from '../utils/format';

export default function FileDropzone({ file, onFile, maxMB = 25, label = 'Drop a file here or click to browse' }) {
  const inputRef = useRef(null);
  const [over, setOver] = useState(false);
  const [error, setError] = useState('');

  const accept = (f) => {
    if (!f) return;
    if (f.size > maxMB * 1024 * 1024) { setError(`File is larger than ${maxMB} MB`); return; }
    setError('');
    onFile(f);
  };

  return (
    <div>
      <div
        role="button" tabIndex={0} onClick={() => inputRef.current?.click()}
        onKeyDown={(e) => (e.key === 'Enter' || e.key === ' ') && inputRef.current?.click()}
        onDragOver={(e) => { e.preventDefault(); setOver(true); }} onDragLeave={() => setOver(false)}
        onDrop={(e) => { e.preventDefault(); setOver(false); accept(e.dataTransfer.files?.[0]); }}
        className={`flex cursor-pointer flex-col items-center justify-center rounded-2xl border-2 border-dashed px-4 py-8 text-center transition ${over ? 'border-brand-400 bg-brand-500/10' : 'border-white/15 bg-white/[0.02] hover:border-white/30'}`}
      >
        <input ref={inputRef} type="file" className="hidden" onChange={(e) => { accept(e.target.files?.[0]); e.target.value = ''; }} />
        <FileUp className="mb-2 h-7 w-7 text-brand-400" />
        {file ? (
          <div className="flex items-center gap-2 text-sm text-slate-200">
            <span className="max-w-[220px] truncate font-medium">{file.name}</span>
            <span className="text-slate-500">{formatBytes(file.size)}</span>
            <button aria-label="Remove file" onClick={(e) => { e.stopPropagation(); onFile(null); }} className="rounded p-0.5 text-slate-400 hover:bg-white/10 hover:text-white"><X className="h-4 w-4" /></button>
          </div>
        ) : (
          <>
            <p className="text-sm text-slate-300">{label}</p>
            <p className="mt-1 text-xs text-slate-500">Up to {maxMB} MB · files are analysed, never executed</p>
          </>
        )}
      </div>
      {error && <p className="mt-2 text-xs text-red-400">{error}</p>}
    </div>
  );
}
