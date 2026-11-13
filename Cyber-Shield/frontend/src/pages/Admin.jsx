import { ShieldAlert, Users } from 'lucide-react';
import Badge from '../components/Badge';
import DashboardCard from '../components/DashboardCard';
import DataTable from '../components/DataTable';
import PageHeader from '../components/PageHeader';
import { useAsync } from '../hooks/useAsync';
import { dashboardService } from '../services/dashboardService';
import { formatDate } from '../utils/format';

export default function Admin() {
  const users = useAsync(dashboardService.users, { immediate: true });
  const global = useAsync(() => dashboardService.get('all'), { immediate: true });
  const g = global.data;

  const columns = [
    { key: 'username', header: 'User', render: (r) => <span className="font-medium text-slate-100">{r.username}</span> },
    { key: 'email', header: 'Email', render: (r) => r.email || <span className="text-slate-600">—</span> },
    { key: 'role', header: 'Role', render: (r) => <Badge tone={r.role === 'admin' ? 'info' : 'neutral'}>{r.role}</Badge> },
    { key: 'created_at', header: 'Registered', render: (r) => formatDate(r.created_at) },
    { key: 'last_login', header: 'Last sign-in', render: (r) => formatDate(r.last_login) },
  ];

  return (
    <>
      <PageHeader icon={ShieldAlert} title="Administration" description="Organisation-wide statistics and user accounts. Visible to administrators only." />
      <div className="mb-6 grid gap-4 sm:grid-cols-2 lg:grid-cols-4">
        <DashboardCard index={0} loading={global.loading && !g} title="Registered users" value={users.data?.length ?? '—'} icon={Users} />
        <DashboardCard index={1} loading={global.loading && !g} title="Org security score" value={g?.securityScore} hint={g?.securityLabel} icon={ShieldAlert} tone="success" />
        <DashboardCard index={2} loading={global.loading && !g} title="Threats (all users)" value={g?.threatCount} icon={ShieldAlert} tone="danger" />
        <DashboardCard index={3} loading={global.loading && !g} title="Critical alerts" value={g?.criticalAlerts} hint="Last 30 days" icon={ShieldAlert} tone="warning" />
      </div>
      <section className="glass overflow-hidden">
        <h3 className="border-b border-white/10 px-5 py-4 font-semibold text-white">User accounts</h3>
        <DataTable columns={columns} rows={users.data || []} loading={users.loading && !users.data} error={users.error && !users.data ? users.error : null} onRetry={users.run} emptyTitle="No users" />
      </section>
    </>
  );
}
