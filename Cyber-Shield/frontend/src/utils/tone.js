// Maps semantic values from the API to Badge tones and colours.
export const severityTone = { info: 'info', warning: 'warning', error: 'danger', critical: 'critical', low: 'info', medium: 'warning', high: 'danger', none: 'neutral' };
export const levelTone = { Low: 'success', Medium: 'warning', High: 'danger', Critical: 'critical' };
export const verdictTone = { Clean: 'success', Suspicious: 'warning', 'High Risk': 'critical' };
export const strengthTone = { 'Very Weak': 'critical', Weak: 'danger', Moderate: 'warning', Strong: 'success', 'Very Strong': 'success' };

export const scoreColor = (score) => (score >= 85 ? '#34d399' : score >= 65 ? '#38bdf8' : score >= 40 ? '#fbbf24' : '#f87171');
export const riskColor = (score) => (score < 20 ? '#34d399' : score < 45 ? '#fbbf24' : score < 70 ? '#fb923c' : '#f87171');
