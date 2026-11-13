/** @type {import('tailwindcss').Config} */
export default {
  content: ['./index.html', './src/**/*.{js,jsx}'],
  theme: {
    extend: {
      colors: {
        ink: { 950: '#070b13', 900: '#0b111c', 800: '#101827', 700: '#182234' },
        brand: { 300: '#7dd3fc', 400: '#38bdf8', 500: '#0ea5e9', 600: '#0284c7' },
      },
      fontFamily: { sans: ['Inter', 'ui-sans-serif', 'system-ui', '-apple-system', 'Segoe UI', 'Roboto', 'sans-serif'], mono: ['JetBrains Mono', 'ui-monospace', 'SFMono-Regular', 'Menlo', 'monospace'] },
      boxShadow: { glow: '0 0 0 1px rgba(56,189,248,.25), 0 8px 30px -8px rgba(14,165,233,.35)' },
    },
  },
  plugins: [],
};
