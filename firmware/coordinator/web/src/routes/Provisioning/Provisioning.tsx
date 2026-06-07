import { useEffect } from 'preact/hooks';
import { useSignal } from '@preact/signals';
import { z } from 'zod';
import { Icon } from '../../components/Icon';
import { WifiPicker } from './components/WifiPicker';
import s from './Provisioning.module.css';

type Step = 'form' | 'success';

const connectErrorSchema = z.enum([
  'none', 'auth_fail', 'ssid_not_found', 'timeout', 'other',
]);
type ConnectError = z.infer<typeof connectErrorSchema>;

const statusSchema = z.object({
  device_id: z.string().min(1),
  name: z.string(),
  uptime_s: z.number().int().nonnegative(),
  wifi_rssi_dbm: z.number().int(),
  mqtt_connected: z.boolean(),
  firmware_version: z.string(),
  ip: z.string(),
  mode: z.enum(['operational', 'provisioning']),
  last_connect_error: connectErrorSchema.optional(),
});

function lastErrorBanner(err: ConnectError | undefined): string | null {
  switch (err) {
    case 'auth_fail':      return 'Last connect failed: wrong password';
    case 'ssid_not_found': return 'Last connect failed: SSID not visible';
    case 'timeout':        return 'Last connect failed: timeout reaching AP';
    case 'other':          return 'Last connect failed: unknown reason';
    default:               return null;
  }
}

export function Provisioning() {
  const step = useSignal<Step>('form');
  const submitting = useSignal(false);
  const error = useSignal<string | null>(null);
  const ssid = useSignal('');
  const lastError = useSignal<ConnectError | undefined>(undefined);

  // Fetch /api/status once on mount to surface the prior STA disconnect
  // reason (auth_fail / ssid_not_found / timeout / other) as a banner above
  // the form. Operational /api/status omits the field entirely, so we use a
  // permissive partial parse: any non-conformant response is ignored.
  useEffect(() => {
    let cancelled = false;
    void (async () => {
      try {
        const res = await fetch('/api/status', { headers: { accept: 'application/json' } });
        if (!res.ok || cancelled) return;
        const raw = await res.json();
        const parsed = statusSchema.safeParse(raw);
        if (parsed.success) {
          lastError.value = parsed.data.last_connect_error;
        } else {
          // Permissive fallback: pluck the field if present and valid.
          const v = (raw as { last_connect_error?: unknown })?.last_connect_error;
          if (typeof v === 'string' &&
              ['none','auth_fail','ssid_not_found','timeout','other'].includes(v)) {
            lastError.value = v as ConnectError;
          }
        }
      } catch {
        // Status fetch is best-effort — banner stays hidden on failure.
      }
    })();
    return () => { cancelled = true; };
  }, []);

  const onSubmit = async (e: SubmitEvent) => {
    e.preventDefault();
    submitting.value = true;
    error.value = null;
    try {
      const form = e.currentTarget as HTMLFormElement;
      const data = new FormData(form);
      const body = new URLSearchParams();
      data.forEach((v, k) => body.append(k, String(v)));
      const res = await fetch('/save', { method: 'POST', body });
      if (!res.ok) {
        const text = await res.text();
        throw new Error(text || `HTTP ${res.status}`);
      }
      step.value = 'success';
    } catch (err) {
      error.value = (err as Error).message;
    } finally {
      submitting.value = false;
    }
  };

  if (step.value === 'success') {
    return (
      <div class={s.prov}>
        <div class={s.hero}>
          <span class={s.successArt}><Icon id="i-check" size={36} /></span>
          <h1 class={s.title}>Settings saved.</h1>
          <p class={s.sub}>
            The coordinator is restarting and will connect to your Wi-Fi.
            It's no longer broadcasting its setup network.
          </p>
          <div class={s.reconnect}>
            <b>What to do now</b>
            <ol>
              <li>Reconnect your phone to your home Wi-Fi.</li>
              <li>Open <span class="mono">http://greenhouse.local</span> &mdash; or look up the device in your router.</li>
              <li>If you don't see anything in 30 seconds, the credentials may be wrong. Press and hold <b>BOOT</b> for 5 s to reopen setup.</li>
            </ol>
          </div>
        </div>
      </div>
    );
  }

  const banner = lastErrorBanner(lastError.value);

  return (
    <div class={s.prov}>
      <div class={s.hero}>
        <span class={s.mark}><Icon id="i-leaf" size={32} /></span>
        <span class={s.step}><span class={s.pip} /> First-time setup</span>
        <h1 class={s.title}>Welcome to your greenhouse.</h1>
        <p class={s.sub}>
          Connect the coordinator to your home Wi-Fi and tell it where Home Assistant lives.
          You'll only do this once.
        </p>
      </div>

      {banner && <div class={s.bannerWarn} role="alert">{banner}</div>}

      <form class={s.form} onSubmit={onSubmit}>
        <div class={s.section}>
          <div class={s.sectionHead}>
            <span class={s.sectionTitle}>Home Wi-Fi</span>
            <span class={s.sectionHint}>2.4 GHz networks only</span>
          </div>
          <WifiPicker selectedSsid={ssid.value} onPick={v => { ssid.value = v; }} />
          <div class={s.field}>
            <label class={s.fieldLbl}><span>Network name (SSID)</span></label>
            <input class={s.input} type="text" name="wifi_ssid" required
                   value={ssid.value}
                   onInput={e => ssid.value = (e.target as HTMLInputElement).value}
                   placeholder="MyHomeWiFi" />
          </div>
          <div class={s.field}>
            <label class={s.fieldLbl}><span>Password</span></label>
            <input class={s.input} type="password" name="wifi_password" required />
          </div>
        </div>

        <div class={s.section}>
          <div class={s.sectionHead}>
            <span class={s.sectionTitle}>Home Assistant (MQTT)</span>
            <span class={s.sectionHint}>optional, can be added later</span>
          </div>
          <div class={s.field}>
            <label class={s.fieldLbl}><span>Broker host</span></label>
            <input class={s.input} type="text" name="mqtt_host" placeholder="homeassistant.local" />
          </div>
          <div class={s.fieldRow}>
            <div class={s.field}>
              <label class={s.fieldLbl}><span>Port</span></label>
              <input class={`${s.input} num`} type="number" name="mqtt_port" value="1883" />
            </div>
            <div class={s.field}>
              <label class={s.fieldLbl}><span>Username</span></label>
              <input class={s.input} type="text" name="mqtt_user" />
            </div>
          </div>
          <div class={s.field}>
            <label class={s.fieldLbl}><span>Password</span></label>
            <input class={s.input} type="password" name="mqtt_password" />
          </div>
        </div>

        <div class={s.section}>
          <div class={s.sectionHead}>
            <span class={s.sectionTitle}>Admin account</span>
            <span class={s.sectionHint}>protects the web interface</span>
          </div>
          <p class={s.sectionDesc}>
            You'll sign in with these to open the dashboard after setup.
          </p>
          <div class={s.field}>
            <label class={s.fieldLbl}><span>Username</span></label>
            <input class={s.input} type="text" name="admin_user" value="admin"
                   autocomplete="username" />
          </div>
          <div class={s.fieldRow}>
            <div class={s.field}>
              <label class={s.fieldLbl}><span>Password</span></label>
              <input class={s.input} type="password" name="admin_password" required
                     minLength={8} autocomplete="new-password"
                     placeholder="min 8 characters" />
            </div>
            <div class={s.field}>
              <label class={s.fieldLbl}><span>Confirm</span></label>
              <input class={s.input} type="password" name="admin_password_confirm" required
                     minLength={8} autocomplete="new-password" />
            </div>
          </div>
        </div>

        <div class={s.section}>
          <div class={s.sectionHead}>
            <span class={s.sectionTitle}>Soil calibration</span>
            <span class={s.sectionHint}>can be tuned later in settings</span>
          </div>
          <p class={s.sectionDesc}>
            Defaults work for most potting mix. Adjust if your readings look wrong.
          </p>
          <div class={s.fieldRow}>
            <div class={s.field}>
              <label class={s.fieldLbl}><span>Dry raw</span></label>
              <input class={`${s.input} num`} type="number" name="soil_dry" value="249" />
            </div>
            <div class={s.field}>
              <label class={s.fieldLbl}><span>Wet raw</span></label>
              <input class={`${s.input} num`} type="number" name="soil_wet" value="489" />
            </div>
          </div>
        </div>

        {error.value && (
          <div class={s.section}>
            <p class={s.errorText}>{error.value}</p>
          </div>
        )}

        <div class={s.actions}>
          <button class={`${s.btn} ${s.isGhost}`} type="button" disabled={submitting.value}>Skip MQTT</button>
          <button class={s.btn} type="submit" disabled={submitting.value}>
            {submitting.value ? 'Saving…' : 'Save & connect'}
          </button>
        </div>
      </form>

      <div class={s.foot}>
        Device <span class="mono">&mdash;</span> &middot; firmware v0.4.0
      </div>
    </div>
  );
}
