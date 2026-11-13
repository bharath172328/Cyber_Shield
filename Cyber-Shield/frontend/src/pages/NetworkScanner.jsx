import { useMemo, useState } from 'react';
import { motion } from 'framer-motion';
import { Network, Radar, ShieldAlert } from 'lucide-react';
import AlertCard from '../components/AlertCard';
import Badge from '../components/Badge';
import Button from '../components/Button';
import DashboardCard from '../components/DashboardCard';
import DataTable from '../components/DataTable';
import Input from '../components/Input';
import PageHeader from '../components/PageHeader';
import { useToast } from '../context/ToastContext';
import { errorMessage } from '../services/api';
import { networkService } from '../services/networkService';
import { severityTone } from '../utils/tone';

const PRESETS = [
  { label: 'Common services', start: 1, end: 1024 },
  { label: 'Web ports', start: 8000, end: 8100 },
  { label: 'Databases', start: 3300, end: 3400 },
];
const stateTone = { open: 'success', closed: 'neutral', filtered: 'warning' };

export default function NetworkScanner() {
  const toast = useToast();
  const [form, setForm] = useState({ host: '127.0.0.1', startPort: 1, endPort: 1024 });
  const [agree, setAgree] = useState(false);
  const [errors, setErrors] = useState({});
  const [result, setResult] = useState(null);
  const [loading, setLoading] = useState(false);
  const [tab, setTab] = useState('open');

  const validate = () => {
    const e = {}; const s = Number(form.startPort), en = Number(form.endPort);
    if (!/^[A-Za-z0-9.-]{1,253}$/.test(form.host.trim())) e.host = 'Enter a valid hostname or IP address';
    if (!Number.isInteger(s) || s < 1 || s > 65535) e.startPort = '1 – 65535';
    if (!Number.isInteger(en) || en < 1 || en > 65535) e.endPort = '1 – 65535';
    else if (Number.isInteger(s) && en < s) e.endPort = 'Must be ≥ start port';
    else if (Number.isInteger(s) && en - s + 1 > 1024) e.endPort = 'Max 1024 ports per scan';
    if (!agree) e.agree = 'You must confirm you are authorised to scan this host';
    setErrors(e); return Object.keys(e).length === 0;
  };

  const scan = async (ev) => {
    ev.preventDefault();
    if (!validate()) return;
    setLoading(true); setResult(null);
    try {
      const r = await networkService.scan({ host: form.host.trim(), startPort: Number(form.startPort), endPort: Number(form.endPort) });
      setResult(r); setTab(r.openCount ? 'open' : 'all');
      toast.success(`Scan complete: ${r.openCount} open port${r.openCount === 1 ? '' : 's'}`);
    } catch (err) { toast.error(errorMessage(err)); }
    finally { setLoading(false); }
  };

  const rows = useMemo(() => (result ? result.ports.filter((p) => tab === 'all' || p.state === tab) : []), [result, tab]);
  const columns = [
    { key: 'port', header: 'Port', render: (r) => <span className="font-mono text-slate-100">{r.port}</span> },
    { key: 'state', header: 'State', render: (r) => <Badge tone={stateTone[r.state]}>{r.state}</Badge> },
    { key: 'service', header: 'Service' },
    { key: 'risk', header: 'Risk', render: (r) => (r.state === 'open' ? <Badge tone={severityTone[r.risk]}>{r.risk}</Badge> : <span className="text-slate-600">—</span>) },
    { key: 'note', header: 'Information', render: (r) => <span className="text-xs text-slate-400">{r.note || '—'}</span> },
  ];

  return (
    <>
      <PageHeader icon={Network} title="Network Scanner" description="Authorised TCP connect scan for systems you own or are permitted to test." />
      <AlertCard tone="warning" title="Authorised use only" className="mb-6">
        Only scan systems you own or have written permission to test. For safety the server restricts scans to loopback and private-network addresses, at most 1024 ports, one scan at a time. Public hosts must be allow-listed by an administrator.
      </AlertCard>

      <form onSubmit={scan} className="glass p-6" noValidate>
        <div className="grid gap-4 sm:grid-cols-6">
          <Input className="sm:col-span-3" label="Host / IP" value={form.host} error={errors.host} onChange={(e) => setForm({ ...form, host: e.target.value })} spellCheck="false" />
          <Input className="sm:col-span-1" label="Start port" type="number" min="1" max="65535" value={form.startPort} error={errors.startPort} onChange={(e) => setForm({ ...form, startPort: e.target.value })} />
          <Input className="sm:col-span-1" label="End port" type="number" min="1" max="65535" value={form.endPort} error={errors.endPort} onChange={(e) => setForm({ ...form, endPort: e.target.value })} />
          <div className="sm:col-span-1 sm:pt-[26px]"><Button type="submit" loading={loading} icon={Radar} className="w-full">Scan</Button></div>
        </div>
        <div className="mt-3 flex flex-wrap gap-2">
          {PRESETS.map((p) => <button key={p.label} type="button" onClick={() => setForm({ ...form, startPort: p.start, endPort: p.end })} className="rounded-full border border-white/10 px-3 py-1 text-xs text-slate-300 hover:bg-white/10">{p.label} · {p.start}-{p.end}</button>)}
        </div>
        <label className="mt-4 flex cursor-pointer items-start gap-2.5 text-sm text-slate-300">
          <input type="checkbox" checked={agree} onChange={(e) => setAgree(e.target.checked)} className="mt-0.5 h-4 w-4 rounded border-white/20 bg-ink-900 accent-sky-500" />
          I confirm I am authorised to scan this host.
        </label>
        {errors.agree && <p className="mt-1.5 text-xs text-red-400">{errors.agree}</p>}
      </form>

      {loading && <div className="glass mt-6 flex items-center justify-center gap-3 p-10 text-sm text-slate-400"><Radar className="h-5 w-5 animate-spin text-brand-400" />Scanning {form.host}…</div>}

      {result && (
        <motion.div initial={{ opacity: 0, y: 12 }} animate={{ opacity: 1, y: 0 }} className="mt-6 space-y-6">
          <div className="grid gap-4 sm:grid-cols-2 lg:grid-cols-4">
            <DashboardCard title="Open" value={result.openCount} hint={`${result.host} (${result.resolvedIp})`} tone="success" icon={Network} />
            <DashboardCard title="Closed" value={result.closedCount} hint="Connection refused" tone="brand" icon={Network} />
            <DashboardCard title="Filtered" value={result.filteredCount} hint="No response" tone="warning" icon={Network} />
            <DashboardCard title="Duration" value={`${result.durationMs} ms`} hint={`${result.riskyOpenCount} risky open port(s)`} tone={result.riskyOpenCount ? 'danger' : 'violet'} icon={ShieldAlert} />
          </div>
          <div className="glass overflow-hidden">
            <div className="flex gap-1 border-b border-white/10 p-3">
              {[['open', `Open (${result.openCount})`], ['closed', `Closed (${result.closedCount})`], ['filtered', `Filtered (${result.filteredCount})`], ['all', `All (${result.ports.length})`]].map(([k, l]) => (
                <button key={k} onClick={() => setTab(k)} className={`rounded-lg px-3 py-1.5 text-xs font-medium ${tab === k ? 'bg-brand-500/15 text-brand-300' : 'text-slate-400 hover:bg-white/5'}`}>{l}</button>
              ))}
            </div>
            <div className="max-h-[480px] overflow-y-auto">
              <DataTable columns={columns} rows={rows} rowKey="port" emptyTitle={`No ${tab === 'all' ? '' : tab} ports`} emptyDescription="Nothing to show for this filter." />
            </div>
          </div>
        </motion.div>
      )}
    </>
  );
}
