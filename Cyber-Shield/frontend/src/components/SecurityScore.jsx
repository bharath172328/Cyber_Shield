import { motion } from 'framer-motion';
import { scoreColor } from '../utils/tone';

export default function SecurityScore({ score = 0, label, size = 160, caption }) {
  const stroke = 12, r = (size - stroke) / 2, c = 2 * Math.PI * r;
  const color = scoreColor(score);
  return (
    <div className="flex flex-col items-center" role="img" aria-label={`Score ${score} out of 100${label ? `, ${label}` : ''}`}>
      <div className="relative" style={{ width: size, height: size }}>
        <svg width={size} height={size} className="-rotate-90">
          <circle cx={size / 2} cy={size / 2} r={r} fill="none" stroke="rgba(255,255,255,.08)" strokeWidth={stroke} />
          <motion.circle
            cx={size / 2} cy={size / 2} r={r} fill="none" stroke={color} strokeWidth={stroke} strokeLinecap="round"
            strokeDasharray={c} initial={{ strokeDashoffset: c }} animate={{ strokeDashoffset: c * (1 - score / 100) }}
            transition={{ duration: 1, ease: 'easeOut' }} style={{ filter: `drop-shadow(0 0 6px ${color}66)` }}
          />
        </svg>
        <div className="absolute inset-0 flex flex-col items-center justify-center">
          <motion.span key={score} initial={{ opacity: 0, scale: 0.8 }} animate={{ opacity: 1, scale: 1 }} className="text-4xl font-semibold text-white">{score}</motion.span>
          <span className="text-xs text-slate-400">/ 100</span>
        </div>
      </div>
      {label && <p className="mt-3 text-sm font-medium" style={{ color }}>{label}</p>}
      {caption && <p className="mt-0.5 text-xs text-slate-500">{caption}</p>}
    </div>
  );
}
