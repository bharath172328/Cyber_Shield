# Cyber Shield — React frontend

React 18 · Vite · Tailwind CSS · React Router · Framer Motion · Recharts · Axios.

```bash
npm install
npm run dev      # http://localhost:5173  (backend expected on :8080)
npm run build    # production bundle in dist/
npm test         # jsdom end-to-end tests (needs the backend running)
```
Structure: `components/` (reusable UI), `pages/` (routes), `layouts/`, `services/` (all API calls), `context/` (auth, toasts), `hooks/`, `utils/`.
Components never call Axios directly — they use the service layer, which normalises errors via `errorMessage()`.
Configure the API address with `VITE_API_URL` (see `.env.example`).
