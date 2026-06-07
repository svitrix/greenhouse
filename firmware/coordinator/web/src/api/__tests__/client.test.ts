import { describe, expect, it, vi, beforeEach } from 'vitest';
import { Api } from '../client';
import {
  setCreds, clearCreds, getCreds, needsLogin, UnauthorizedError,
} from '../client';

const sampleNodes = {
  ts_ms: 1000,
  nodes: [{
    ieee: '00124B001A2B3C4D',
    short_addr: '0x1A2B',
    alias: 'Tomatoes',
    online: true,
    last_seen_s: 12,
    rssi_dbm: -52,
    proto_version: 1,
    proto_version_mismatch: false,
    present_mask: '0x07',
    readings: [
      { kind: 'air', quantity: 'temp_c', value: 23.4, unit: '°C', age_s: 12 },
    ],
  }],
};

describe('Api.listNodes', () => {
  it('parses a valid response', async () => {
    const fetchImpl = vi.fn().mockResolvedValue({
      ok: true, status: 200,
      headers: new Headers({ 'content-type': 'application/json' }),
      json: () => Promise.resolve(sampleNodes),
    });
    const api = new Api(fetchImpl as unknown as typeof window.fetch);
    const r = await api.listNodes();
    expect(r.nodes).toHaveLength(1);
    expect(r.nodes[0].alias).toBe('Tomatoes');
  });

  it('throws on a malformed response', async () => {
    const fetchImpl = vi.fn().mockResolvedValue({
      ok: true, status: 200,
      headers: new Headers({ 'content-type': 'application/json' }),
      json: () => Promise.resolve({ nodes: 'bad' }),
    });
    const api = new Api(fetchImpl as unknown as typeof window.fetch);
    await expect(api.listNodes()).rejects.toThrow();
  });
});

describe('Api auth', () => {
  beforeEach(() => {
    sessionStorage.clear();
    clearCreds();
    needsLogin.value = false;
  });

  const okNodes = () => vi.fn().mockResolvedValue({
    ok: true, status: 200, json: () => Promise.resolve(sampleNodes),
  });

  it('attaches the Authorization header when credentials are stored', async () => {
    setCreds({ user: 'admin', pass: 'x' });
    const fetchImpl = okNodes();
    await new Api(fetchImpl as unknown as typeof window.fetch).listNodes();
    const opts = fetchImpl.mock.calls[0][1] as RequestInit;
    expect((opts.headers as Record<string, string>).authorization)
      .toBe('Basic ' + btoa('admin:x'));
  });

  it('omits the Authorization header when no credentials', async () => {
    const fetchImpl = okNodes();
    await new Api(fetchImpl as unknown as typeof window.fetch).listNodes();
    const opts = fetchImpl.mock.calls[0][1] as RequestInit;
    expect((opts.headers as Record<string, string>).authorization).toBeUndefined();
  });

  it('on 401 throws UnauthorizedError, clears creds and flips needsLogin', async () => {
    setCreds({ user: 'admin', pass: 'x' });
    const fetchImpl = vi.fn().mockResolvedValue({ ok: false, status: 401 });
    const api = new Api(fetchImpl as unknown as typeof window.fetch);
    await expect(api.listNodes()).rejects.toBeInstanceOf(UnauthorizedError);
    expect(getCreds()).toBeNull();
    expect(needsLogin.value).toBe(true);
  });
});
