import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { render, screen } from '@testing-library/preact';
import { needsLogin, clearCreds, setCreds } from '../../api/auth';

beforeEach(() => {
  // Default: a never-resolving fetch keeps App in its "Loading…" state unless a
  // test re-stubs it. Reset routing + auth signals between tests.
  vi.stubGlobal('fetch', vi.fn(() => new Promise(() => { /* never resolves */ })));
  location.hash = '';
  sessionStorage.clear();
  clearCreds();
  needsLogin.value = false;
});

afterEach(() => {
  vi.unstubAllGlobals();
});

import { App } from '../App';

describe('App', () => {
  it('shows the loading state until the mode probe resolves', () => {
    render(<App />);
    expect(screen.getByText(/Loading/)).toBeInTheDocument();
  });

  it('shows Login when /api/status returns 401 (operational, unauthenticated)', async () => {
    vi.stubGlobal('fetch', vi.fn().mockResolvedValue({ ok: false, status: 401 }));
    render(<App />);
    expect(await screen.findByRole('button', { name: /sign in/i })).toBeInTheDocument();
  });

  it('renders Provisioning on mode:provisioning without requiring login', async () => {
    vi.stubGlobal('fetch', vi.fn().mockResolvedValue({
      ok: true, status: 200, json: () => Promise.resolve({ mode: 'provisioning' }),
    }));
    render(<App />);
    expect(await screen.findByText(/Welcome to your greenhouse/i)).toBeInTheDocument();
    expect(screen.queryByRole('button', { name: /sign in/i })).not.toBeInTheDocument();
  });

  it('goes to the dashboard when operational and already authenticated', async () => {
    setCreds({ user: 'admin', pass: 'x' });
    // /api/status resolves operational; subsequent dashboard polls never resolve.
    vi.stubGlobal('fetch', vi.fn().mockImplementation((url: string) =>
      url.includes('/api/status')
        ? Promise.resolve({ ok: true, status: 200,
            json: () => Promise.resolve({ mode: 'operational' }) })
        : new Promise(() => { /* never resolves */ })));
    render(<App />);
    // Dashboard's own loading state, not the login form.
    expect(await screen.findByText(/Loading/)).toBeInTheDocument();
    expect(screen.queryByRole('button', { name: /sign in/i })).not.toBeInTheDocument();
  });
});
