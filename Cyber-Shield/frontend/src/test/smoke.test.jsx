// End-to-end smoke test: real React app + real C++ API + real SQLite database.
// Requires the backend on http://localhost:8080 with a registered user (see docs).
import { cleanup, fireEvent, render, screen, waitFor } from '@testing-library/react';
import { MemoryRouter } from 'react-router-dom';
import { afterEach, beforeAll, describe, expect, it } from 'vitest';
import App from './../App';
import { AuthProvider } from '../context/AuthContext';
import { ToastProvider } from '../context/ToastContext';
import { TOKEN_KEY } from '../services/api';

const USER = process.env.CS_TEST_USER || 'alice';
const PASS = process.env.CS_TEST_PASS || 'Str0ng!Passphrase42';

beforeAll(() => {
  globalThis.ResizeObserver = class { observe() {} unobserve() {} disconnect() {} };
  window.HTMLElement.prototype.scrollIntoView = () => {};
});
afterEach(() => { cleanup(); localStorage.clear(); });

const mount = (path) => render(
  <MemoryRouter initialEntries={[path]}><ToastProvider><AuthProvider><App /></AuthProvider></ToastProvider></MemoryRouter>,
);

async function loginToken() {
  const r = await fetch('http://localhost:8080/api/auth/login', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ username: USER, password: PASS }) });
  return (await r.json()).token;
}

describe('Cyber Shield frontend', () => {
  it('redirects unauthenticated users to /login', async () => {
    mount('/dashboard');
    expect(await screen.findByRole('heading', { name: /sign in/i })).toBeTruthy();
  });

  it('validates the login form client-side', async () => {
    mount('/login');
    fireEvent.click(await screen.findByRole('button', { name: /^sign in$/i }));
    expect(await screen.findByText('Username is required')).toBeTruthy();
  });

  it('shows the API error for wrong credentials', async () => {
    mount('/login');
    fireEvent.change(await screen.findByLabelText(/username/i), { target: { value: USER } });
    fireEvent.change(screen.getByLabelText(/^password/i), { target: { value: 'wrong-password' } });
    fireEvent.click(screen.getByRole('button', { name: /^sign in$/i }));
    expect(await screen.findByText('Invalid username or password')).toBeTruthy();
  });

  it('logs in through the UI and loads live dashboard data (frontend -> C++ API -> SQLite)', async () => {
    mount('/login');
    fireEvent.change(await screen.findByLabelText(/username/i), { target: { value: USER } });
    fireEvent.change(screen.getByLabelText(/^password/i), { target: { value: PASS } });
    fireEvent.click(screen.getByRole('button', { name: /^sign in$/i }));
    expect(await screen.findByText('Security Overview')).toBeTruthy();
    expect(await screen.findByText('Threats detected')).toBeTruthy();
    await waitFor(() => expect(screen.getByRole('img', { name: /score \d+ out of 100/i })).toBeTruthy());
  });

  const pages = [
    ['/password-analyzer', 'Password Analyzer'], ['/url-analyzer', 'URL & Phishing Analyzer'], ['/network-scanner', 'Network Scanner'],
    ['/file-security', 'File Security'], ['/file-integrity', 'File Integrity'], ['/security-logs', 'Security Logs'],
    ['/reports', 'Threat Reports'], ['/settings', 'Settings'], ['/admin', 'Administration'],
  ];
  it.each(pages)('renders %s for an authenticated admin', async (path, title) => {
    localStorage.setItem(TOKEN_KEY, await loginToken());
    mount(path);
    expect(await screen.findByRole('heading', { name: title })).toBeTruthy();
  });

  it('runs a password analysis end to end', async () => {
    localStorage.setItem(TOKEN_KEY, await loginToken());
    mount('/password-analyzer');
    fireEvent.change(await screen.findByLabelText(/password to analyse/i), { target: { value: 'Tr0ub4dor&3xyz!' } });
    fireEvent.click(screen.getByRole('button', { name: /analyse password/i }));
    expect(await screen.findByText('Very Strong')).toBeTruthy();
  });

  it('runs a URL analysis end to end', async () => {
    localStorage.setItem(TOKEN_KEY, await loginToken());
    mount('/url-analyzer');
    fireEvent.change(await screen.findByLabelText(/^url$/i), { target: { value: 'http://paypal.com.secure-login.verify-account.xyz/signin' } });
    fireEvent.click(screen.getByRole('button', { name: /^analyse$/i }));
    expect(await screen.findByText(/critical risk/i)).toBeTruthy();
  });

  it('runs a network scan end to end and enforces the authorisation checkbox', async () => {
    localStorage.setItem(TOKEN_KEY, await loginToken());
    mount('/network-scanner');
    fireEvent.click(await screen.findByRole('button', { name: /^scan$/i }));
    expect(await screen.findByText(/must confirm you are authorised/i)).toBeTruthy();
    fireEvent.click(screen.getByRole('checkbox'));
    fireEvent.click(screen.getByRole('button', { name: /^scan$/i }));
    expect(await screen.findByText('Duration')).toBeTruthy();
  });

  it('generates a report end to end', async () => {
    localStorage.setItem(TOKEN_KEY, await loginToken());
    mount('/reports');
    fireEvent.click((await screen.findAllByRole('button', { name: /generate report/i }))[0]);
    expect(await screen.findByText('Recommendations')).toBeTruthy();
  });

  it('redirects a non-admin away from /admin', async () => {
    const r = await fetch('http://localhost:8080/api/auth/login', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ username: 'bob', password: 'Another#Str0ng99pw' }) });
    const { token } = await r.json();
    localStorage.setItem(TOKEN_KEY, token);
    mount('/admin');
    expect(await screen.findByText('Security Overview')).toBeTruthy();
  });
});
