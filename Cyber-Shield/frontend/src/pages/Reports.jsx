import { useState } from 'react';
import { FileBarChart, FilePlus2, FileText } from 'lucide-react';
import AlertCard from '../components/AlertCard';
import Badge from '../components/Badge';
import Button from '../components/Button';
import EmptyState from '../components/EmptyState';
import ErrorState from '../components/ErrorState';
import Modal from '../components/Modal';
import PageHeader from '../components/PageHeader';
import SecurityScore from '../components/SecurityScore';
import Skeleton from '../components/Skeleton';
import { useToast } from '../context/ToastContext';
import { useAsync } from '../hooks/useAsync';
import { errorMessage } from '../services/api';
import { dashboardService } from '../services/dashboardService';
import { formatDate } from '../utils/format';
import { scoreColor } from '../utils/tone';

export default function Reports() {
  const toast = useToast();
  const { data: items, loading, error, run: reload } = useAsync(dashboardService.reports, { immediate: true });
  const [creating, setCreating] = useState(false);
  const [selected, setSelected] = useState(null);

  const generate = async () => {
    setCreating(true);
    try {
      const r = await dashboardService.generateReport(`Security Report — ${new Date().toLocaleDateString()}`);
      toast.success('Report generated'); await reload(); setSelected(r);
    } catch (err) { toast.error(errorMessage(err)); } finally { setCreating(false); }
  };

  const s = selected?.summary;
  return (
    <>
      <PageHeader icon={FileBarChart} title="Threat Reports" description="Point-in-time snapshots of your security posture with recommended actions."
        actions={<Button icon={FilePlus2} loading={creating} onClick={generate}>Generate report</Button>} />

      <div className="glass overflow-hidden">
        {error && !items ? <ErrorState message={error} onRetry={reload} /> : loading && !items ? (
          <div className="space-y-3 p-5">{Array.from({ length: 3 }).map((_, i) => <Skeleton key={i} className="h-14" />)}</div>
        ) : items.length === 0 ? (
          <EmptyState icon={FileText} title="No reports yet" description="Generate your first report to capture the current security score and recommendations." action={<Button icon={FilePlus2} loading={creating} onClick={generate}>Generate report</Button>} />
        ) : (
          <ul className="divide-y divide-white/5">
            {items.map((r) => (
              <li key={r.id}>
                <button onClick={() => setSelected(r)} className="flex w-full items-center gap-4 px-5 py-4 text-left hover:bg-white/[0.03]">
                  <span className="rounded-xl bg-white/5 p-2.5 text-brand-400"><FileText className="h-5 w-5" /></span>
                  <div className="min-w-0 flex-1"><p className="truncate font-medium text-slate-100">{r.title}</p><p className="text-xs text-slate-500">{formatDate(r.createdAt)}</p></div>
                  <span className="text-lg font-semibold tabular-nums" style={{ color: scoreColor(r.summary.securityScore) }}>{r.summary.securityScore}</span>
                  <Badge tone={r.summary.threatCount ? 'danger' : 'success'}>{r.summary.threatCount} threats</Badge>
                </button>
              </li>
            ))}
          </ul>
        )}
      </div>

      <Modal open={!!selected} onClose={() => setSelected(null)} title={selected?.title || ''} size="lg">
        {s && (
          <div className="space-y-6">
            <div className="flex flex-col items-center gap-6 sm:flex-row">
              <SecurityScore score={s.securityScore} label={s.securityLabel} size={130} />
              <dl className="grid flex-1 grid-cols-2 gap-3 text-sm sm:grid-cols-3">
                {[['Threats', s.threatCount], ['Critical alerts', s.criticalAlerts], ['Passwords tested', s.passwordStats.total], ['URLs analysed', s.urlStats.total], ['Network scans', s.networkStats.total], ['Files scanned', s.fileStats.total]].map(([k, v]) => (
                  <div key={k} className="rounded-xl border border-white/10 bg-white/[0.03] p-3"><dt className="text-xs text-slate-500">{k}</dt><dd className="text-xl font-semibold text-white">{v}</dd></div>
                ))}
              </dl>
            </div>
            <div>
              <h4 className="mb-2 font-semibold text-white">Recommendations</h4>
              <div className="space-y-2">{s.recommendations.map((r) => <AlertCard key={r} tone={s.threatCount ? 'warning' : 'success'}>{r}</AlertCard>)}</div>
            </div>
          </div>
        )}
      </Modal>
    </>
  );
}
