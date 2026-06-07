import { useEffect, useState } from 'preact/hooks';
import type { ConfigView } from '../../api/types';
import { getApi } from '../../state/useDashboard';
import s from './Settings.module.css';

// Route config through the shared, auth-aware Api so the Authorization header is
// attached and a 401 routes back to login (was a raw unauthenticated fetch that
// broke under Basic Auth).
const fetchConfig = (): Promise<ConfigView> => getApi().getConfig();
const postConfig = (body: unknown): Promise<void> => getApi().setConfig(body);

export function Settings() {
  const [cfg, setCfg] = useState<ConfigView | null>(null);
  const [status, setStatus] = useState<string>('');

  useEffect(() => {
    void fetchConfig().then(setCfg).catch((e) => setStatus(`error: ${e}`));
  }, []);

  if (!cfg) return <div class={s.loading}>{status || 'Loading…'}</div>;

  const aw = cfg.auto_water;
  const update = (patch: Partial<typeof aw>) => {
    setCfg({ ...cfg, auto_water: { ...aw, ...patch } });
  };

  const save = async () => {
    try {
      await postConfig({ auto_water: cfg.auto_water });
      setStatus('Saved');
    } catch (e) {
      setStatus(`error: ${e}`);
    }
  };

  return (
    <div class={s.settings}>
      <a href="#/" class={s.back}>← Back</a>
      <h2>Settings</h2>
      <h3>Auto-water</h3>
      <label>
        Enabled
        <input type="checkbox" checked={aw.enabled}
               onInput={(e) => update({
                 enabled: (e.target as HTMLInputElement).checked,
               })} />
      </label>
      <label>
        Trigger below %
        <input type="number" value={aw.trigger_below_pct} min={5} max={80}
               onInput={(e) => update({
                 trigger_below_pct: parseInt((e.target as HTMLInputElement).value, 10),
               })} />
      </label>
      <label>
        Duration s
        <input type="number" value={aw.duration_s} min={1} max={20}
               onInput={(e) => update({
                 duration_s: parseInt((e.target as HTMLInputElement).value, 10),
               })} />
      </label>
      <label>
        Min interval min
        <input type="number" value={aw.min_interval_min} min={5} max={1440}
               onInput={(e) => update({
                 min_interval_min: parseInt((e.target as HTMLInputElement).value, 10),
               })} />
      </label>
      <label>
        Min fresh sources
        <input type="number" value={aw.min_fresh_sources} min={1} max={8}
               onInput={(e) => update({
                 min_fresh_sources: parseInt((e.target as HTMLInputElement).value, 10),
               })} />
      </label>
      <label>
        Stale threshold s
        <input type="number" value={aw.stale_threshold_s} min={30} max={3600}
               onInput={(e) => update({
                 stale_threshold_s: parseInt((e.target as HTMLInputElement).value, 10),
               })} />
      </label>
      <button onClick={() => void save()}>Save</button>

      <h3>MQTT (read-only)</h3>
      <div>host: {cfg.mqtt.host || '—'}</div>
      <div>port: {cfg.mqtt.port}</div>
      <div>user: {cfg.mqtt.user || '—'}</div>
      <div>password set: {cfg.mqtt.password_set ? 'yes' : 'no'}</div>

      <h3>Wi-Fi (read-only)</h3>
      <div>SSID: {cfg.wifi.ssid || '—'}</div>

      {status && <div class={s.status}>{status}</div>}
    </div>
  );
}
