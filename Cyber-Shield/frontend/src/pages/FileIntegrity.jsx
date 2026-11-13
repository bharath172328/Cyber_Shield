import { useState } from 'react';
import { motion } from 'framer-motion';
import { BadgeCheck, FileCheck2, Trash2, ShieldCheck } from 'lucide-react';
import AlertCard from '../components/AlertCard';
import Badge from '../components/Badge';
import Button from '../components/Button';
import CopyButton from '../components/CopyButton';
import DataTable from '../components/DataTable';
import FileDropzone from '../components/FileDropzone';
import Input from '../components/Input';
import { ConfirmDialog } from '../components/Modal';
import PageHeader from '../components/PageHeader';
import { useToast } from '../context/ToastContext';
import { useAsync } from '../hooks/useAsync';
import { errorMessage } from '../services/api';
import { fileService } from '../services/fileService';
import { formatDate } from '../utils/format';

const statusTone = { baseline: 'info', intact: 'success', modified: 'critical' };

export default function FileIntegrity() {
  const toast = useToast();
  const { data: items, loading, error, run: reload } = useAsync(fileService.listBaselines, { immediate: true });
  const [baseFile, setBaseFile] = useState(null);
  const [verFile, setVerFile] = useState(null);
  const [expected, setExpected] = useState('');
  const [busy, setBusy] = useState('');
  const [result, setResult] = useState(null);
  const [toDelete, setToDelete] = useState(null);
  const [deleting, setDeleting] = useState(false);

  const createBaseline = async () => {
    setBusy('baseline');
    try { await fileService.createBaseline(baseFile); toast.success(`Baseline saved for ${baseFile.name}`); setBaseFile(null); reload().catch(() => {}); }
    catch (err) { toast.error(errorMessage(err)); } finally { setBusy(''); }
  };
  const verify = async () => {
    setBusy('verify'); setResult(null);
    try { setResult(await fileService.verify(verFile, expected.trim())); reload().catch(() => {}); }
    catch (err) { toast.error(errorMessage(err)); } finally { setBusy(''); }
  };
  const confirmDelete = async () => {
    setDeleting(true);
    try { await fileService.deleteBaseline(toDelete.id); toast.success('Baseline deleted'); setToDelete(null); reload().catch(() => {}); }
    catch (err) { toast.error(errorMessage(err)); } finally { setDeleting(false); }
  };

  const columns = [
    { key: 'filename', header: 'File', render: (r) => <span className="break-all font-medium text-slate-100">{r.filename}</span> },
    { key: 'status', header: 'Status', render: (r) => <Badge tone={statusTone[r.status]}>{r.status}</Badge> },
    { key: 'baseline_hash', header: 'Baseline SHA-256', render: (r) => <span className="flex items-center gap-1"><code className="font-mono text-xs text-slate-400">{r.baseline_hash.slice(0, 16)}…</code><CopyButton value={r.baseline_hash} /></span> },
    { key: 'last_checked', header: 'Last checked', render: (r) => formatDate(r.last_checked) },
    { key: 'created_at', header: 'Created', render: (r) => formatDate(r.created_at) },
    { key: 'actions', header: '', render: (r) => <Button variant="ghost" size="sm" icon={Trash2} aria-label={`Delete baseline for ${r.filename}`} onClick={() => setToDelete(r)} /> },
  ];

  const tone = result && (result.status === 'intact' ? 'success' : result.status === 'modified' ? 'critical' : 'warning');

  return (
    <>
      <PageHeader icon={FileCheck2} title="File Integrity" description="Store a trusted SHA-256 baseline, then verify later that the file has not been modified." />
      <div className="grid gap-6 lg:grid-cols-2">
        <section className="glass space-y-4 p-6">
          <h3 className="font-semibold text-white">1 · Create baseline</h3>
          <FileDropzone file={baseFile} onFile={setBaseFile} label="Choose the trusted version of the file" />
          <Button onClick={createBaseline} loading={busy === 'baseline'} disabled={!baseFile} icon={BadgeCheck}>Save baseline</Button>
          <p className="text-xs text-slate-500">Saving a baseline for an existing file name replaces its previous baseline.</p>
        </section>

        <section className="glass space-y-4 p-6">
          <h3 className="font-semibold text-white">2 · Verify a file</h3>
          <FileDropzone file={verFile} onFile={(f) => { setVerFile(f); setResult(null); }} label="Choose the file to check" />
          <Input label="Compare against a known SHA-256 (optional)" value={expected} onChange={(e) => setExpected(e.target.value)} placeholder="Leave empty to use the stored baseline" spellCheck="false" />
          <Button onClick={verify} loading={busy === 'verify'} disabled={!verFile} icon={ShieldCheck}>Verify integrity</Button>
          {result && (
            <motion.div initial={{ opacity: 0, y: 8 }} animate={{ opacity: 1, y: 0 }}>
              <AlertCard tone={tone} title={result.status === 'intact' ? 'File is intact' : result.status === 'modified' ? 'File has been modified' : 'No baseline found'}>
                {result.status === 'no_baseline' ? 'Create a baseline for this file name first, or paste an expected hash to compare against.' : (
                  <div className="space-y-1 font-mono text-xs break-all">
                    <p><span className="text-slate-500">expected </span>{result.baselineHash}</p>
                    <p><span className="text-slate-500">current&nbsp; </span>{result.currentHash}</p>
                  </div>
                )}
              </AlertCard>
            </motion.div>
          )}
        </section>
      </div>

      <section className="glass mt-6 overflow-hidden">
        <h3 className="border-b border-white/10 px-5 py-4 font-semibold text-white">Stored baselines</h3>
        <DataTable columns={columns} rows={items || []} loading={loading && !items} error={error && !items ? error : null} onRetry={reload} emptyTitle="No baselines yet" emptyDescription="Create a baseline to start monitoring a file." />
      </section>

      <ConfirmDialog open={!!toDelete} onClose={() => setToDelete(null)} onConfirm={confirmDelete} loading={deleting} title="Delete baseline?" confirmLabel="Delete"
        message={<>The stored hash for <strong className="text-white">{toDelete?.filename}</strong> will be permanently removed.</>} />
    </>
  );
}
