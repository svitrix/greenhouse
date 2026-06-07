import { signal } from '@preact/signals';

// Single source of truth for admin credentials + "must show login" state.
// The device protects /api/* with HTTP Basic Auth; browsers do NOT surface the
// native Basic-Auth dialog for fetch()/XHR, so the SPA must attach the header
// itself from credentials the user enters in an in-app login form.

const KEY = 'gh.auth';

export interface Creds { user: string; pass: string; }

// When true, App renders <Login> instead of the operational routes.
export const needsLogin = signal<boolean>(false);

// Set when a 401 rejects credentials that WERE present (i.e. wrong password),
// so <Login> can show an error. Survives the App re-render to <Login>.
export const loginError = signal<boolean>(false);

// In-memory fallback for environments where sessionStorage throws (e.g. Safari
// private mode) — credentials still work for the tab's lifetime.
let mem: Creds | null = null;

export function getCreds(): Creds | null {
  if (mem) return mem;
  try {
    const raw = sessionStorage.getItem(KEY);
    return raw ? (JSON.parse(raw) as Creds) : null;
  } catch {
    return mem;
  }
}

export function setCreds(c: Creds): void {
  mem = c;
  try { sessionStorage.setItem(KEY, JSON.stringify(c)); } catch { /* keep mem */ }
  loginError.value = false;
  needsLogin.value = false;
}

export function clearCreds(): void {
  mem = null;
  try { sessionStorage.removeItem(KEY); } catch { /* ignore */ }
}

// "Basic base64(user:pass)" or null when no credentials are stored.
// utf-8 encode first so non-ASCII passwords survive btoa()'s latin1 input.
export function basicHeader(): string | null {
  const c = getCreds();
  if (!c) return null;
  return 'Basic ' + btoa(unescape(encodeURIComponent(`${c.user}:${c.pass}`)));
}

export class UnauthorizedError extends Error {
  constructor() {
    super('Unauthorized');
    this.name = 'UnauthorizedError';
  }
}
