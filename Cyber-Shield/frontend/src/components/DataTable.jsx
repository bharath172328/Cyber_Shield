import { motion } from 'framer-motion';
import EmptyState from './EmptyState';
import ErrorState from './ErrorState';
import Skeleton from './Skeleton';

/** columns: [{ key, header, render?(row), className? }] — scrolls horizontally on small screens. */
export default function DataTable({ columns, rows = [], loading, error, onRetry, emptyTitle, emptyDescription, rowKey = 'id' }) {
  if (error) return <ErrorState message={error} onRetry={onRetry} />;
  return (
    <div className="overflow-x-auto">
      <table className="w-full min-w-[640px] text-left text-sm">
        <thead>
          <tr className="border-b border-white/10 text-xs uppercase tracking-wide text-slate-500">
            {columns.map((c) => <th key={c.key} className={`px-4 py-3 font-medium ${c.className || ''}`}>{c.header}</th>)}
          </tr>
        </thead>
        <tbody>
          {loading && Array.from({ length: 5 }).map((_, i) => (
            <tr key={i} className="border-b border-white/5">{columns.map((c) => <td key={c.key} className="px-4 py-3"><Skeleton className="h-4 w-full max-w-[160px]" /></td>)}</tr>
          ))}
          {!loading && rows.map((row, i) => (
            <motion.tr key={row[rowKey] ?? i} initial={{ opacity: 0 }} animate={{ opacity: 1 }} transition={{ delay: Math.min(i, 10) * 0.02 }} className="border-b border-white/5 hover:bg-white/[0.03]">
              {columns.map((c) => <td key={c.key} className={`px-4 py-3 align-top text-slate-300 ${c.className || ''}`}>{c.render ? c.render(row) : row[c.key]}</td>)}
            </motion.tr>
          ))}
        </tbody>
      </table>
      {!loading && rows.length === 0 && <EmptyState title={emptyTitle} description={emptyDescription} />}
    </div>
  );
}
