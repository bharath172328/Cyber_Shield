import { useState } from 'react';
import { motion } from 'framer-motion';
import { Globe, Link2, Search } from 'lucide-react';
import AlertCard from '../components/AlertCard';
import Badge from '../components/Badge';
import Button from '../components/Button';
import Input from '../components/Input';
import PageHeader from '../components/PageHeader';
import { useToast } from '../context/ToastContext';
import { errorMessage } from '../services/api';
import { urlService } from '../services/urlService';
import { levelTone, riskColor, severityTone } from '../utils/tone';

const EXAMPLES = [
  'https://www.wikipedia.org/wiki/Cyber_security',
  'http://paypal.com.secure-login.verify-account.xyz/signin',
  'http://192.168.10.5/admin/login.php',
];

export default function UrlAnalyzer() {
  const toast = useToast();
  const [url, setUrl] = useState('');
  const [result, setResult] = useState(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');

  const analyze = async (value = url) => {
    if (!value.trim()) { setError('Enter a URL to analyse'); return; }
    setError(''); setLoading(true);
    try { setResult(await urlService.analyze(value.trim())); }
    catch (err) { const m = errorMessage(err); if (err.response?.status === 400) setError(m); else toast.error(m); }
    finally { setLoading(false); }
  };

  const tone = result && (result.riskLevel === 'Low' ? 'success' : result.riskLevel === 'Medium' ? 'warning' : 'critical');

  return (
    <>
      <PageHeader icon={Link2} title="URL & Phishing Analyzer" description="Static analysis of the link's structure. Cyber Shield never visits the URL — it only inspects the text." />
      <form onSubmit={(e) => { e.preventDefault(); analyze(); }} className="glass p-6">
        <div className="flex flex-col gap-3 sm:flex-row sm:items-start">
          <Input className="flex-1" icon={Globe} label="URL" value={url} error={error} onChange={(e) => { setUrl(e.target.value); setError(''); }} placeholder="https://example.com/login" spellCheck="false" />
          <Button type="submit" loading={loading} icon={Search} className="sm:mt-[26px]">Analyse</Button>
        </div>
        <div className="mt-3 flex flex-wrap items-center gap-2 text-xs text-slate-500">
          Try an example:
          {EXAMPLES.map((ex) => <button key={ex} type="button" onClick={() => { setUrl(ex); analyze(ex); }} className="max-w-[240px] truncate rounded-full border border-white/10 px-3 py-1 text-slate-300 hover:bg-white/10">{ex}</button>)}
        </div>
      </form>

      {result && (
        <motion.div key={result.url} initial={{ opacity: 0, y: 12 }} animate={{ opacity: 1, y: 0 }} className="mt-6 grid gap-6 lg:grid-cols-3">
          <div className="glass p-6">
            <p className="text-xs font-medium uppercase tracking-wide text-slate-400">Risk score</p>
            <p className="mt-2 text-5xl font-semibold text-white">{result.riskScore}<span className="text-xl text-slate-500"> / 100</span></p>
            <div className="mt-4 h-2 overflow-hidden rounded-full bg-white/10">
              <motion.div className="h-full rounded-full" style={{ background: riskColor(result.riskScore) }} initial={{ width: 0 }} animate={{ width: `${result.riskScore}%` }} transition={{ duration: 0.8 }} />
            </div>
            <div className="mt-4"><Badge tone={levelTone[result.riskLevel]} className="text-sm">{result.riskLevel} risk</Badge></div>
            <dl className="mt-6 space-y-2 text-sm">
              {[['Scheme', result.scheme], ['Host', result.host], ['Port', result.port], ['Path', result.path]].map(([k, v]) => (
                <div key={k} className="flex justify-between gap-4"><dt className="text-slate-500">{k}</dt><dd className="truncate font-mono text-slate-300">{v}</dd></div>
              ))}
            </dl>
          </div>
          <div className="space-y-4 lg:col-span-2">
            <AlertCard tone={tone} title="Recommendation">{result.recommendation}</AlertCard>
            <div className="glass p-6">
              <h3 className="mb-3 font-semibold text-white">Detected indicators ({result.indicators.length})</h3>
              {result.indicators.length === 0 ? <p className="text-sm text-slate-500">No suspicious indicators were found in the URL structure.</p> : (
                <ul className="space-y-2.5">
                  {result.indicators.map((i, idx) => (
                    <motion.li key={i.id} initial={{ opacity: 0, x: -8 }} animate={{ opacity: 1, x: 0 }} transition={{ delay: idx * 0.04 }} className="flex items-start gap-3 rounded-xl border border-white/10 bg-white/[0.02] p-3">
                      <Badge tone={severityTone[i.severity]}>{i.severity}</Badge>
                      <p className="flex-1 text-sm text-slate-300">{i.description}</p>
                      <span className="text-xs tabular-nums text-slate-500">+{i.weight}</span>
                    </motion.li>
                  ))}
                </ul>
              )}
            </div>
          </div>
        </motion.div>
      )}
    </>
  );
}
