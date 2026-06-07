import { useEffect } from 'preact/hooks';
import { useSignal } from '@preact/signals';
import { z } from 'zod';
import { Icon } from '../../../../components/Icon';
import s from './WifiPicker.module.css';

const wifiScanItemSchema = z.object({
  ssid: z.string(),
  rssi: z.number().int(),
});
const wifiScanSchema = z.object({
  networks: z.array(wifiScanItemSchema),
});
type WifiScanItem = z.infer<typeof wifiScanItemSchema>;

type Props = {
  selectedSsid: string;
  onPick: (ssid: string) => void;
};

type State = { items: WifiScanItem[]; loading: boolean; error: string | null };

const RSSI_CLASS: Record<string, string> = {
  strong: s.isStrong, good: s.isGood, fair: s.isFair, weak: s.isWeak,
};

function rssiKey(dbm: number): 'strong' | 'good' | 'fair' | 'weak' {
  if (dbm > -50) return 'strong';
  if (dbm > -65) return 'good';
  if (dbm > -75) return 'fair';
  return 'weak';
}

function dedupe(list: WifiScanItem[]): WifiScanItem[] {
  const map = new Map<string, WifiScanItem>();
  for (const n of list) {
    if (!n.ssid) continue;
    const prev = map.get(n.ssid);
    if (!prev || n.rssi > prev.rssi) map.set(n.ssid, n);
  }
  return [...map.values()].sort((a, b) => b.rssi - a.rssi);
}

export function WifiPicker({ selectedSsid, onPick }: Props) {
  const state = useSignal<State>({ items: [], loading: true, error: null });

  const scan = async () => {
    state.value = { ...state.value, loading: true, error: null };
    try {
      const res = await fetch('/scan', { headers: { accept: 'application/json' } });
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const raw = await res.json();
      const parsed = wifiScanSchema.parse(raw);
      state.value = { items: dedupe(parsed.networks), loading: false, error: null };
    } catch (e) {
      state.value = { items: [], loading: false, error: (e as Error).message };
    }
  };

  useEffect(() => { void scan(); }, []);

  const { items, loading, error } = state.value;

  return (
    <div class={s.netlist}>
      <div class={s.head}>
        <span class={s.title}>Available networks</span>
        <button class={s.rescan} type="button" onClick={() => void scan()} disabled={loading}>
          <Icon id="i-refresh" size={14} /> {loading ? 'Scanning…' : 'Rescan'}
        </button>
      </div>

      {error && (
        <div class={s.error}>Scan failed: {error}. Type the SSID manually below.</div>
      )}

      {!error && !loading && items.length === 0 && (
        <div class={s.empty}>No networks visible. Move the device closer to your router and rescan.</div>
      )}

      {!error && items.length > 0 && (
        <ul class={s.rows}>
          {items.map(n => {
            const cls = [s.row];
            if (n.ssid === selectedSsid) cls.push(s.isSelected);
            const rssiCls = RSSI_CLASS[rssiKey(n.rssi)];
            return (
              <li>
                <button class={cls.join(' ')} type="button" onClick={() => onPick(n.ssid)}>
                  <span class={s.ssid}>{n.ssid}</span>
                  <span class={`${s.rssi} ${rssiCls}`} title={`${n.rssi} dBm`}>
                    {rssiKey(n.rssi)}
                  </span>
                </button>
              </li>
            );
          })}
        </ul>
      )}
    </div>
  );
}
