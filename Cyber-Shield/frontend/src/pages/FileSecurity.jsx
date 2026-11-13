import { useState } from 'react';
import { motion } from 'framer-motion';
import { FileSearch, ScanSearch } from 'lucide-react';
import AlertCard from '../components/AlertCard';
import Badge from '../components/Badge';
import Button from '../components/Button';
import CopyButton from '../components/CopyButton';
import FileDropzone from '../components/FileDropzone';
import PageHeader from '../components/PageHeader';
import SecurityScore from '../components/SecurityScore';
import { useToast } from '../context/ToastContext';
import { errorMessage } from '../services/api';
import { fileService } from '../services/fileService';
import { formatBytes } from '../utils/format';
import { severityTone, verdictTone } from '../utils/tone';

export default function FileSecurity() {
  const toast = useToast();
  const [file, setFile] = useState(null);
  const [hash, setHash] = useState(null);
  const [analysis, setAnalysis] = useState(null);
  const [loading, setLoading] = useState(false);

  const scan = async () => {
    if (!file) { toast.warning('Choose a file first'); return; }
    setLoading(true); setHash(null); setAnalysis(null);
    try {
      const [h, a] = await Promise.all([fileService.hash(file), fileService.analyze(file)]);
      setHash(h); setAnalysis(a);
    } catch (err) { toast.error(errorMessage(err)); }
    finally { setLoading(false); }
  };

  const tone = analysis && (analysis.verdict === 'Clean' ? 'success' : analysis.verdict === 'Suspicious' ? 'warning' : 'critical');

  return (
    <>
      <PageHeader icon={FileSearch} title="File Security" description="Calculate cryptographic hashes and inspect a file for suspicious traits. Files are analysed statically and never executed." />
      <div className="glass space-y-4 p-6">
        <FileDropzone file={file} onFile={(f) => { setFile(f); setHash(null); setAnalysis(null); }} />
        <Button onClick={scan} loading={loading} icon={ScanSearch} disabled={!file}>Hash &amp; analyse</Button>
      </div>

      {hash && analysis && (
        <motion.div initial={{ opacity: 0, y: 12 }} animate={{ opacity: 1, y: 0 }} className="mt-6 grid gap-6 lg:grid-cols-3">
          <div className="glass flex flex-col items-center p-6 text-center">
            <SecurityScore score={100 - analysis.riskScore} size={140} label={analysis.verdict} caption={`Risk score ${analysis.riskScore}`} />
            <Badge tone={verdictTone[analysis.verdict]} className="mt-4 text-sm">{analysis.verdict}</Badge>
          </div>

          <div className="space-y-6 lg:col-span-2">
            <div className="glass p-6">
              <h3 className="mb-4 font-semibold text-white">File details</h3>
              <dl className="grid gap-3 text-sm sm:grid-cols-2">
                <div><dt className="text-slate-500">File name</dt><dd className="break-all text-slate-200">{hash.filename}</dd></div>
                <div><dt className="text-slate-500">Size</dt><dd className="text-slate-200">{formatBytes(hash.size)} ({hash.size.toLocaleString()} bytes)</dd></div>
                <div><dt className="text-slate-500">Detected type</dt><dd className="text-slate-200">{analysis.fileType}</dd></div>
                <div><dt className="text-slate-500">Entropy</dt><dd className="text-slate-200">{analysis.entropy} / 8.00</dd></div>
              </dl>
              <div className="mt-5 space-y-2">
                {[['SHA-256', hash.hashes.sha256, true], ['SHA-1', hash.hashes.sha1], ['MD5', hash.hashes.md5]].map(([n, v, main]) => (
                  <div key={n} className="flex items-center gap-2 rounded-xl border border-white/10 bg-ink-900/60 px-3 py-2">
                    <span className="w-16 shrink-0 text-xs font-medium text-slate-400">{n}</span>
                    <code className="min-w-0 flex-1 break-all font-mono text-xs text-slate-200">{v}</code>
                    {main && <Badge tone="success">Recommended</Badge>}
                    <CopyButton value={v} />
                  </div>
                ))}
              </div>
              <p className="mt-3 text-xs text-slate-500">{hash.md5Note}</p>
            </div>

            <div className="glass p-6">
              <h3 className="mb-3 font-semibold text-white">Analysis findings ({analysis.findings.length})</h3>
              {analysis.findings.length === 0 ? <AlertCard tone="success">No suspicious traits detected. That is not a guarantee of safety — keep antivirus software enabled.</AlertCard> : (
                <ul className="space-y-2.5">
                  {analysis.findings.map((f) => (
                    <li key={f.id} className="flex items-start gap-3 rounded-xl border border-white/10 bg-white/[0.02] p-3">
                      <Badge tone={severityTone[f.severity]}>{f.severity}</Badge>
                      <p className="flex-1 text-sm text-slate-300">{f.description}</p>
                      <span className="text-xs tabular-nums text-slate-500">+{f.weight}</span>
                    </li>
                  ))}
                </ul>
              )}
              {tone && analysis.findings.length > 0 && <AlertCard tone={tone} className="mt-4" title="Recommendation">{analysis.verdict === 'High Risk' ? 'Do not open or run this file. Quarantine it and verify where it came from.' : 'Treat this file with caution and confirm its source before opening it.'}</AlertCard>}
            </div>
          </div>
        </motion.div>
      )}
    </>
  );
}
