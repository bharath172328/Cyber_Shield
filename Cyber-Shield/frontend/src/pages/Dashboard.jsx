import { Activity, AlertOctagon, FileSearch, KeyRound, Link2, Network, RefreshCw, ShieldAlert } from 'lucide-react';
import { Area, AreaChart, Bar, BarChart, CartesianGrid, Cell, Legend, Pie, PieChart, ResponsiveContainer, Tooltip, XAxis, YAxis } from 'recharts';
import AlertCard from '../components/AlertCard';
import Badge from '../components/Badge';
import Button from '../components/Button';
import ChartCard from '../components/ChartCard';
import DashboardCard from '../components/DashboardCard';
import EmptyState from '../components/EmptyState';
import ErrorState from '../components/ErrorState';
import PageHeader from '../components/PageHeader';
import SecurityScore from '../components/SecurityScore';
import SecurityStatus from '../components/SecurityStatus';
import Skeleton from '../components/Skeleton';
import ThreatCard from '../components/ThreatCard';
import { useAsync } from '../hooks/useAsync';
import { dashboardService } from '../services/dashboardService';
import { eventLabel, shortDay, timeAgo } from '../utils/format';
import { severityTone } from '../utils/tone';

const tip = { contentStyle: { background: '#101827', border: '1px solid rgba(255,255,255,.1)', borderRadius: 12, fontSize: 12 }, labelStyle: { color: '#94a3b8' }, cursor: { fill: 'rgba(255,255,255,.04)' } };
const axis = { stroke: '#64748b', fontSize: 12, tickLine: false, axisLine: false };
const LEVEL_COLORS = { Low: '#34d399', Medium: '#fbbf24', High: '#fb923c', Critical: '#f87171' };
const STRENGTH_COLORS = { 'Very Weak': '#f87171', Weak: '#fb923c', Moderate: '#fbbf24', Strong: '#38bdf8', 'Very Strong': '#34d399' };

export default function Dashboard() {
  const { data: d, loading, error, run } = useAsync(() => dashboardService.get(), { immediate: true });
  const trend = (d?.trend || []).map((t) => ({ ...t, day: shortDay(t.date) }));
  const urlPie = Object.entries(d?.urlStats?.byLevel || {}).map(([name, value]) => ({ name, value }));
  const pwBars = Object.entries(d?.passwordStats?.byStrength || {}).map(([name, value]) => ({ name, value }));
  const totalActivity = d ? d.passwordStats.total + d.urlStats.total + d.networkStats.total + d.fileStats.total : 0;

  return (
    <>
      <PageHeader title="Security Overview" description="Live posture across every Cyber Shield module."
        actions={<div className="flex items-center gap-3">{d && <SecurityStatus status={d.securityLabel} />}<Button variant="secondary" size="sm" icon={RefreshCw} onClick={() => run().catch(() => {})} loading={loading && !!d}>Refresh</Button></div>} />

      {error && !d ? <div className="glass"><ErrorState message={error} onRetry={() => run().catch(() => {})} /></div> : (
        <div className="space-y-6">
          {d?.criticalAlerts > 0 && <AlertCard tone="critical" title={`${d.criticalAlerts} critical alert${d.criticalAlerts > 1 ? 's' : ''} in the last 30 days`}>Review the security logs to investigate what triggered them.</AlertCard>}

          <div className="grid gap-4 sm:grid-cols-2 xl:grid-cols-4">
            <div className="glass row-span-1 flex items-center justify-center p-5 sm:col-span-2 xl:col-span-1 xl:row-span-2">
              {!d ? <Skeleton className="h-40 w-40 rounded-full" /> : <SecurityScore score={d.securityScore} label={d.securityLabel} caption="Overall security score" />}
            </div>
            <DashboardCard index={0} loading={!d} title="Threats detected" value={d?.threatCount} hint="High-risk URLs, files & exposed ports" icon={ShieldAlert} tone="danger" />
            <DashboardCard index={1} loading={!d} title="Critical alerts" value={d?.criticalAlerts} hint="Last 30 days" icon={AlertOctagon} tone="warning" />
            <DashboardCard index={2} loading={!d} title="Total analyses" value={totalActivity} hint="Across all modules" icon={Activity} tone="brand" />
            <DashboardCard index={3} loading={!d} title="Avg. password score" value={d?.passwordStats?.total ? d.passwordStats.averageScore : '—'} hint={`${d?.passwordStats?.weak ?? 0} weak passwords tested`} icon={KeyRound} tone="success" />
            <DashboardCard index={4} loading={!d} title="Open ports found" value={d?.networkStats?.openPorts} hint={`${d?.networkStats?.riskyOpenPorts ?? 0} risky`} icon={Network} tone="violet" />
          </div>

          <div className="grid gap-4 sm:grid-cols-2 xl:grid-cols-4">
            <ThreatCard index={0} loading={!d} title="Passwords" icon={KeyRound} total={d?.passwordStats?.total} threats={d?.passwordStats?.weak} unit="tested" />
            <ThreatCard index={1} loading={!d} title="URLs" icon={Link2} total={d?.urlStats?.total} threats={d?.urlStats?.highRisk} unit="analysed" />
            <ThreatCard index={2} loading={!d} title="Network scans" icon={Network} total={d?.networkStats?.openPorts} threats={d?.networkStats?.riskyOpenPorts} unit="open ports" />
            <ThreatCard index={3} loading={!d} title="Files" icon={FileSearch} total={d?.fileStats?.total} threats={d?.fileStats?.threats} unit="scanned" />
          </div>

          <div className="grid gap-4 lg:grid-cols-3">
            <ChartCard className="lg:col-span-2" title="Activity — last 7 days" subtitle="Analyses run per module" loading={!d}>
              <ResponsiveContainer width="100%" height="100%">
                <AreaChart data={trend} margin={{ left: -20, right: 8, top: 5 }}>
                  <defs>
                    {[['p', '#38bdf8'], ['u', '#818cf8'], ['s', '#34d399'], ['f', '#fbbf24']].map(([id, c]) => (
                      <linearGradient key={id} id={`g-${id}`} x1="0" y1="0" x2="0" y2="1"><stop offset="5%" stopColor={c} stopOpacity={0.35} /><stop offset="95%" stopColor={c} stopOpacity={0} /></linearGradient>
                    ))}
                  </defs>
                  <CartesianGrid stroke="rgba(255,255,255,.06)" vertical={false} />
                  <XAxis dataKey="day" {...axis} /><YAxis allowDecimals={false} {...axis} />
                  <Tooltip {...tip} /><Legend wrapperStyle={{ fontSize: 12 }} />
                  <Area type="monotone" dataKey="passwords" name="Passwords" stroke="#38bdf8" fill="url(#g-p)" strokeWidth={2} />
                  <Area type="monotone" dataKey="urls" name="URLs" stroke="#818cf8" fill="url(#g-u)" strokeWidth={2} />
                  <Area type="monotone" dataKey="scans" name="Scans" stroke="#34d399" fill="url(#g-s)" strokeWidth={2} />
                  <Area type="monotone" dataKey="files" name="Files" stroke="#fbbf24" fill="url(#g-f)" strokeWidth={2} />
                </AreaChart>
              </ResponsiveContainer>
            </ChartCard>

            <ChartCard title="URL risk levels" subtitle="Distribution of analysed URLs" loading={!d}>
              {urlPie.length === 0 ? <EmptyState title="No URLs analysed" description="Analyse a URL to see its risk mix." /> : (
                <ResponsiveContainer width="100%" height="100%">
                  <PieChart>
                    <Pie data={urlPie} dataKey="value" nameKey="name" innerRadius={55} outerRadius={85} paddingAngle={3} stroke="none">
                      {urlPie.map((e) => <Cell key={e.name} fill={LEVEL_COLORS[e.name] || '#64748b'} />)}
                    </Pie>
                    <Tooltip {...tip} /><Legend wrapperStyle={{ fontSize: 12 }} />
                  </PieChart>
                </ResponsiveContainer>
              )}
            </ChartCard>
          </div>

          <div className="grid gap-4 lg:grid-cols-3">
            <ChartCard title="Threat trend" subtitle="Flagged items per day" loading={!d}>
              <ResponsiveContainer width="100%" height="100%">
                <BarChart data={trend} margin={{ left: -20, right: 8, top: 5 }}>
                  <CartesianGrid stroke="rgba(255,255,255,.06)" vertical={false} />
                  <XAxis dataKey="day" {...axis} /><YAxis allowDecimals={false} {...axis} />
                  <Tooltip {...tip} />
                  <Bar dataKey="threats" name="Threats" fill="#f87171" radius={[6, 6, 0, 0]} />
                </BarChart>
              </ResponsiveContainer>
            </ChartCard>

            <ChartCard title="Password strength" subtitle="Tested passwords by rating" loading={!d}>
              {pwBars.length === 0 ? <EmptyState title="No passwords tested" /> : (
                <ResponsiveContainer width="100%" height="100%">
                  <BarChart data={pwBars} layout="vertical" margin={{ left: 10, right: 12 }}>
                    <XAxis type="number" allowDecimals={false} {...axis} /><YAxis type="category" dataKey="name" width={80} {...axis} />
                    <Tooltip {...tip} />
                    <Bar dataKey="value" name="Passwords" radius={[0, 6, 6, 0]}>{pwBars.map((e) => <Cell key={e.name} fill={STRENGTH_COLORS[e.name] || '#38bdf8'} />)}</Bar>
                  </BarChart>
                </ResponsiveContainer>
              )}
            </ChartCard>

            <section className="glass p-5">
              <h3 className="mb-3 font-semibold text-white">Recent security events</h3>
              {!d ? <div className="space-y-3">{Array.from({ length: 5 }).map((_, i) => <Skeleton key={i} className="h-10" />)}</div> :
                (d.recentEvents || []).length === 0 ? <EmptyState title="No events yet" description="Run an analysis to generate activity." /> : (
                  <ul className="max-h-[260px] space-y-3 overflow-y-auto pr-1">
                    {(d.recentEvents || []).map((e) => (
                      <li key={e.id} className="flex items-start gap-3">
                        <Badge tone={severityTone[e.severity]} className="mt-0.5">{e.severity}</Badge>
                        <div className="min-w-0 flex-1"><p className="truncate text-sm text-slate-200">{eventLabel(e.event_type)}</p><p className="truncate text-xs text-slate-500">{e.description}</p></div>
                        <span className="shrink-0 text-[11px] text-slate-500">{timeAgo(e.created_at)}</span>
                      </li>
                    ))}
                  </ul>
                )}
            </section>
          </div>
        </div>
      )}
    </>
  );
}
