import { useEffect, useState } from 'preact/hooks';
import { Dashboard } from '../routes/Dashboard/Dashboard';
import { NodeDetail } from '../routes/NodeDetail/NodeDetail';
import { Pair } from '../routes/Pair/Pair';
import { Settings } from '../routes/Settings/Settings';
import { Provisioning } from '../routes/Provisioning/Provisioning';
import { Login } from '../routes/Login/Login';
import { needsLogin, clearCreds, basicHeader } from '../api/auth';

type Mode = 'unknown' | 'provisioning' | 'operational';

export function App() {
  const [hash, setHash] = useState(location.hash || '#/');
  const [mode, setMode] = useState<Mode>('unknown');
  const login = needsLogin.value;  // subscribe to the auth signal

  useEffect(() => {
    const h = () => setHash(location.hash || '#/');
    addEventListener('hashchange', h);
    return () => removeEventListener('hashchange', h);
  }, []);

  // Probe /api/status to pick the mode. It is auth-gated in operational mode
  // but open in provisioning, so the HTTP status disambiguates:
  //   401            -> operational, must log in
  //   200 + prov     -> provisioning (no login)
  //   200 + oper     -> operational, already authenticated
  useEffect(() => {
    let cancelled = false;
    void (async () => {
      const auth = basicHeader();
      // Time-box the probe: on a flaky Wi-Fi link the request can hang, which
      // would otherwise leave the app stuck on "Loading…" forever. On
      // timeout/abort we fall through to operational so Login can render.
      const ctrl = new AbortController();
      const t = setTimeout(() => ctrl.abort(), 5000);
      const r = await fetch('/api/status', {
        headers: { accept: 'application/json', ...(auth ? { authorization: auth } : {}) },
        signal: ctrl.signal,
      }).catch(() => null);
      clearTimeout(t);
      if (cancelled) return;
      if (!r) { setMode('operational'); return; }
      if (r.status === 401) {
        clearCreds();
        needsLogin.value = true;
        setMode('operational');
        return;
      }
      if (r.ok) {
        const j = await r.json().catch(() => null);
        setMode(j?.mode === 'provisioning' ? 'provisioning' : 'operational');
        return;
      }
      setMode('operational');
    })();
    return () => { cancelled = true; };
  }, []);

  // Explicit setup link always works, regardless of detected mode.
  if (hash === '#/setup' || hash.startsWith('#/setup/')) return <Provisioning />;

  if (mode === 'unknown') return <div class="app-splash">Loading…</div>;
  if (mode === 'provisioning') return <Provisioning />;
  if (login) return <Login />;

  if (hash === '#/pair') return <Pair />;
  if (hash === '#/settings') return <Settings />;
  if (hash.startsWith('#/nodes/')) {
    const ieee = hash.replace('#/nodes/', '').replace(/\/.*/, '');
    return <NodeDetail ieee={ieee} />;
  }
  return <Dashboard />;
}
