import { useEffect, useState } from 'react';
import { RefreshCw, ScrollText, Search } from 'lucide-react';
import Badge from '../components/Badge';
import Button from '../components/Button';
import DataTable from '../components/DataTable';
import Input from '../components/Input';
import PageHeader from '../components/PageHeader';
import Pagination from '../components/Pagination';
import { useAsync } from '../hooks/useAsync';
import { useDebounce } from '../hooks/useDebounce';
import { useAuth } from '../context/AuthContext';
import { dashboardService } from '../services/dashboardService';
import { eventLabel, formatDate } from '../utils/format';
import { severityTone } from '../utils/tone';

const TYPES = ['LOGIN_SUCCESS', 'LOGIN_FAILED', 'REGISTER', 'PASSWORD_ANALYSIS', 'URL_ANALYSIS', 'NETWORK_SCAN', 'FILE_HASH', 'FILE_ANALYSIS', 'FILE_INTEGRITY', 'SECURITY_EVENT', 'REPORT', 'ERROR', 'SYSTEM'];
const PAGE_SIZE = 15;

export default function SecurityLogs() {
  const { isAdmin } = useAuth();
  const [q, setQ] = useState('');
  const [severity, setSeverity] = useState('');
  const [type, setType] = useState('');
  const [all, setAll] = useState(false);
  const [page, setPage] = useState(1);
  const dq = useDebounce(q);

  const { data, loading, error, run } = useAsync(
    () => dashboardService.logs({ page, pageSize: PAGE_SIZE, q: dq || undefined, severity: severity || undefined, type: type || undefined, scope: all ? 'all' : undefined }),
    { immediate: true, deps: [page, dq, severity, type, all] },
  );
  useEffect(() => { setPage(1); }, [dq, severity, type, all]);

  const columns = [
    { key: 'created_at', header: 'Time', className: 'whitespace-nowrap', render: (r) => formatDate(r.created_at) },
    { key: 'severity', header: 'Severity', render: (r) => <Badge tone={severityTone[r.severity]}>{r.severity}</Badge> },
    { key: 'event_type', header: 'Event', className: 'whitespace-nowrap', render: (r) => eventLabel(r.event_type) },
    { key: 'description', header: 'Description', className: 'min-w-[260px]' },
    { key: 'username', header: 'User', render: (r) => r.username || <span className="text-slate-600">system</span> },
    { key: 'ip_address', header: 'IP', render: (r) => <span className="font-mono text-xs text-slate-500">{r.ip_address || '—'}</span> },
  ];

  return (
    <>
      <PageHeader icon={ScrollText} title="Security Logs" description="Every login, analysis, scan and system event recorded by Cyber Shield."
        actions={<Button variant="secondary" size="sm" icon={RefreshCw} onClick={() => run()}>Refresh</Button>} />
      <section className="glass overflow-hidden">
        <div className="grid gap-3 border-b border-white/10 p-4 sm:grid-cols-2 lg:grid-cols-4">
          <Input icon={Search} placeholder="Search descriptions…" value={q} onChange={(e) => setQ(e.target.value)} aria-label="Search logs" />
          <select className="field" value={severity} onChange={(e) => setSeverity(e.target.value)} aria-label="Filter by severity">
            <option value="">All severities</option>{['info', 'warning', 'error', 'critical'].map((s) => <option key={s} value={s}>{s}</option>)}
          </select>
          <select className="field" value={type} onChange={(e) => setType(e.target.value)} aria-label="Filter by event type">
            <option value="">All event types</option>{TYPES.map((t) => <option key={t} value={t}>{eventLabel(t)}</option>)}
          </select>
          {isAdmin && <label className="flex items-center gap-2 text-sm text-slate-300"><input type="checkbox" checked={all} onChange={(e) => setAll(e.target.checked)} className="h-4 w-4 accent-sky-500" />All users (admin)</label>}
        </div>
        <DataTable columns={columns} rows={data?.items || []} loading={loading && !data} error={error && !data ? error : null} onRetry={run} emptyTitle="No matching log entries" emptyDescription="Try clearing the filters or run an analysis to generate events." />
        <Pagination page={page} pageSize={PAGE_SIZE} total={data?.total || 0} onChange={setPage} />
      </section>
    </>
  );
}
