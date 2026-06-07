import { useEffect, useState } from 'preact/hooks';
import type { NodesResponse, AutoWaterState, PumpView, DashboardView } from '../api/types';
import { Api } from '../api/client';
import { DashboardSchema } from '../api/schemas';

const api = new Api();

type Data = {
  nodes?: NodesResponse;
  pump?: PumpView;
  auto?: AutoWaterState;
  error?: string;
};

// Dashboard data via Server-Sent Events (one persistent connection, server
// pushes ~every 2 s) instead of polling 3 endpoints every 2 s — far less churn
// on the Wi-Fi/Zigbee-shared radio. Falls back to polling GET /api/dashboard
// if SSE is unavailable or never connects.
export function useDashboard(interval_ms = 2000): Data {
  const [data, setData] = useState<Data>({});
  useEffect(() => {
    let alive = true;
    let es: EventSource | null = null;
    let pollTimer: ReturnType<typeof setTimeout> | null = null;
    let fallbackTimer: ReturnType<typeof setTimeout> | null = null;

    // Map the combined payload into the flat shape Dashboard.tsx expects.
    const apply = (d: DashboardView) => {
      if (alive) {
        setData({ nodes: { ts_ms: d.ts_ms, nodes: d.nodes }, pump: d.pump, auto: d.auto });
      }
    };

    const poll = async () => {
      if (!alive) return;
      if (!document.hidden) {
        try { apply(await api.getDashboard()); }
        catch (e) { if (alive) setData((prev) => ({ ...prev, error: String(e) })); }
      }
      pollTimer = setTimeout(poll, interval_ms);
    };
    const startFallbackPolling = () => { if (!pollTimer && alive) void poll(); };

    // Initial snapshot so the dashboard renders before the first SSE tick.
    void (async () => {
      try { apply(await api.getDashboard()); }
      catch (e) { if (alive) setData((prev) => ({ ...prev, error: String(e) })); }
    })();

    if (typeof EventSource !== 'undefined') {
      es = new EventSource('/api/events');
      const onMsg = (ev: MessageEvent) => {
        // First push proves SSE works → cancel any fallback / pending poll.
        if (fallbackTimer) { clearTimeout(fallbackTimer); fallbackTimer = null; }
        if (pollTimer) { clearTimeout(pollTimer); pollTimer = null; }
        try { apply(DashboardSchema.parse(JSON.parse(ev.data))); }
        catch (e) { if (alive) setData((prev) => ({ ...prev, error: String(e) })); }
      };
      es.addEventListener('dashboard', onMsg as EventListener);
      es.onmessage = onMsg;
      // If no SSE message lands within 4× the cadence, degrade to polling.
      fallbackTimer = setTimeout(startFallbackPolling, interval_ms * 4);
    } else {
      void poll();
    }

    return () => {
      alive = false;
      if (es) es.close();
      if (pollTimer) clearTimeout(pollTimer);
      if (fallbackTimer) clearTimeout(fallbackTimer);
    };
  }, [interval_ms]);
  return data;
}

export function getApi(): Api { return api; }
