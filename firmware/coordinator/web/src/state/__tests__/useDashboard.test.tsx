import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { render, screen, waitFor } from '@testing-library/preact';
import { useDashboard } from '../useDashboard';

// jsdom has no EventSource — install a controllable mock.
class MockEventSource {
  static last: MockEventSource | null = null;
  url: string;
  onmessage: ((ev: MessageEvent) => void) | null = null;
  private listeners: Record<string, ((ev: MessageEvent) => void)[]> = {};
  constructor(url: string) { this.url = url; MockEventSource.last = this; }
  addEventListener(type: string, cb: (ev: MessageEvent) => void) {
    (this.listeners[type] ||= []).push(cb);
  }
  close() {}
  emit(type: string, data: string) {
    const ev = { data } as MessageEvent;
    (this.listeners[type] || []).forEach((f) => f(ev));
    if (type === 'message' && this.onmessage) this.onmessage(ev);
  }
}

const sample = {
  ts_ms: 1,
  nodes: [{
    ieee: '00124B001A2B3C4D', short_addr: '0x1A2B', alias: null, online: true,
    last_seen_s: 3, rssi_dbm: -50, proto_version: 1, proto_version_mismatch: false,
    present_mask: '0x07', readings: [],
  }],
  pump: { state: 'OFF', remaining_s: 0, last_run_ms: 0 },
  auto: { avg_moisture_pct: null, fresh_sources: [], stale_sources: [],
          last_decision_ms: 0, last_decision: 'disabled' },
};

function Harness() {
  const d = useDashboard();
  return <div>count:{d.nodes ? d.nodes.nodes.length : 'none'} pump:{d.pump ? d.pump.state : 'none'}</div>;
}

beforeEach(() => {
  vi.stubGlobal('EventSource', MockEventSource as unknown as typeof EventSource);
  // initial GET /api/dashboard never resolves, so the SSE push is what updates.
  vi.stubGlobal('fetch', vi.fn(() => new Promise(() => { /* never */ })));
});
afterEach(() => { vi.unstubAllGlobals(); });

describe('useDashboard (SSE)', () => {
  it('updates state from a pushed "dashboard" event', async () => {
    render(<Harness />);
    MockEventSource.last!.emit('dashboard', JSON.stringify(sample));
    await waitFor(() =>
      expect(screen.getByText(/count:1 pump:OFF/)).toBeInTheDocument());
  });

  it('opens the SSE connection to /api/events', () => {
    render(<Harness />);
    expect(MockEventSource.last?.url).toBe('/api/events');
  });
});
