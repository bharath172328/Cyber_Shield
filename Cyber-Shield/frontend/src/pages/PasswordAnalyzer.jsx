import { useState } from 'react';
import { motion } from 'framer-motion';
import { CheckCircle2, Eye, EyeOff, KeyRound, ShieldCheck, XCircle } from 'lucide-react';
import Badge from '../components/Badge';
import Button from '../components/Button';
import Input from '../components/Input';
import PageHeader from '../components/PageHeader';
import SecurityScore from '../components/SecurityScore';
import { useToast } from '../context/ToastContext';
import { errorMessage } from '../services/api';
import { passwordService } from '../services/passwordService';
import { strengthTone } from '../utils/tone';

export default function PasswordAnalyzer() {
  const toast = useToast();
  const [password, setPassword] = useState('');
  const [show, setShow] = useState(false);
  const [result, setResult] = useState(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');

  const analyze = async (e) => {
    e.preventDefault();
    if (!password) { setError('Enter a password to analyse'); return; }
    if (password.length > 256) { setError('Password must be 256 characters or fewer'); return; }
    setError(''); setLoading(true);
    try { setResult(await passwordService.analyze(password)); }
    catch (err) { toast.error(errorMessage(err)); }
    finally { setLoading(false); }
  };

  return (
    <>
      <PageHeader icon={KeyRound} title="Password Analyzer" description="Check length, character variety, sequences and common patterns. Passwords are never stored or logged — only the score is kept." />
      <div className="grid gap-6 lg:grid-cols-5">
        <form onSubmit={analyze} className="glass space-y-4 p-6 lg:col-span-2">
          <Input label="Password to analyse" type={show ? 'text' : 'password'} autoComplete="off" spellCheck="false" value={password} error={error}
            onChange={(e) => { setPassword(e.target.value); setError(''); }} placeholder="Type or paste a password"
            right={<button type="button" onClick={() => setShow(!show)} aria-label={show ? 'Hide password' : 'Show password'} className="rounded-lg p-1.5 text-slate-400 hover:text-white">{show ? <EyeOff className="h-4 w-4" /> : <Eye className="h-4 w-4" />}</button>} />
          <Button type="submit" loading={loading} icon={ShieldCheck} className="w-full">Analyse password</Button>
          <p className="text-xs text-slate-500">Tip: a passphrase of 4+ random words is easier to remember and harder to crack than a short complex password.</p>
        </form>

        <div className="lg:col-span-3">
          {!result ? (
            <div className="glass flex h-full min-h-[280px] items-center justify-center p-6 text-center text-sm text-slate-500">Results will appear here after analysis.</div>
          ) : (
            <motion.div key={result.score + result.strength} initial={{ opacity: 0, y: 12 }} animate={{ opacity: 1, y: 0 }} className="glass p-6">
              <div className="flex flex-col items-center gap-6 sm:flex-row">
                <SecurityScore score={result.score} size={150} />
                <div className="text-center sm:text-left">
                  <Badge tone={strengthTone[result.strength]} className="text-sm">{result.strength}</Badge>
                  <p className="mt-2 text-lg font-medium text-white">{result.message}</p>
                  <p className="mt-1 text-sm text-slate-400">Estimated entropy: <span className="font-mono text-slate-200">{result.entropyBits} bits</span></p>
                </div>
              </div>
              <ul className="mt-6 grid gap-2 sm:grid-cols-2">
                {result.checks.map((c, i) => (
                  <motion.li key={c.name} initial={{ opacity: 0, x: -8 }} animate={{ opacity: 1, x: 0 }} transition={{ delay: i * 0.04 }}
                    className={`flex items-start gap-2.5 rounded-xl border p-3 text-sm ${c.passed ? 'border-emerald-500/15 bg-emerald-500/5' : 'border-red-500/20 bg-red-500/5'}`}>
                    {c.passed ? <CheckCircle2 className="mt-0.5 h-4 w-4 shrink-0 text-emerald-400" /> : <XCircle className="mt-0.5 h-4 w-4 shrink-0 text-red-400" />}
                    <div><p className="font-medium text-slate-200">{c.name}</p><p className="text-xs text-slate-500">{c.detail}</p></div>
                  </motion.li>
                ))}
              </ul>
            </motion.div>
          )}
        </div>
      </div>
    </>
  );
}
