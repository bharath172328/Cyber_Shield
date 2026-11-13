const styles = {
  Secure: 'border-emerald-500/30 bg-emerald-500/10 text-emerald-300',
  Good: 'border-sky-500/30 bg-sky-500/10 text-sky-300',
  'At Risk': 'border-amber-500/30 bg-amber-500/10 text-amber-300',
  Critical: 'border-red-500/30 bg-red-500/10 text-red-300',
};
const dots = { Secure: 'bg-emerald-400', Good: 'bg-sky-400', 'At Risk': 'bg-amber-400', Critical: 'bg-red-400' };

export default function SecurityStatus({ status = 'Secure' }) {
  return (
    <span className={`inline-flex items-center gap-2 rounded-full border px-3 py-1 text-xs font-medium ${styles[status] || styles.Good}`}>
      <span className="relative flex h-2 w-2">
        <span className={`absolute inline-flex h-full w-full animate-ping rounded-full opacity-60 ${dots[status]}`} />
        <span className={`relative inline-flex h-2 w-2 rounded-full ${dots[status]}`} />
      </span>
      System {status}
    </span>
  );
}
