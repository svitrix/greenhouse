import {
  NodesResponseSchema, NodeViewSchema,
  PumpViewSchema, AutoWaterStateSchema, DashboardSchema,
} from './schemas';
import type {
  NodesResponse, NodeView, PumpView, AutoWaterState, ConfigView, DashboardView,
} from './types';
import {
  basicHeader, clearCreds, getCreds, needsLogin, loginError, UnauthorizedError,
} from './auth';

export class Api {
  constructor(private fetchImpl: typeof window.fetch = window.fetch.bind(window)) {}

  async listNodes(): Promise<NodesResponse> {
    return this.getJson('/api/nodes', (raw) => NodesResponseSchema.parse(raw));
  }

  async getNode(ieee: string): Promise<NodeView> {
    return this.getJson(`/api/nodes/${ieee}`, (raw) => NodeViewSchema.parse(raw));
  }

  async setAlias(ieee: string, alias: string): Promise<void> {
    await this.putJson(`/api/nodes/${ieee}/alias`, { alias });
  }

  async deleteNode(ieee: string): Promise<{ ok: boolean; leave_acked: boolean }> {
    const r = await this.fetchImpl(`/api/nodes/${ieee}`,
      { method: 'DELETE', headers: this.authHeaders() });
    if (!r.ok) this.fail(r.status);
    return r.json() as Promise<{ ok: boolean; leave_acked: boolean }>;
  }

  async getPump(): Promise<PumpView> {
    return this.getJson('/api/pump', (raw) => PumpViewSchema.parse(raw));
  }

  async setPump(state: 'ON' | 'OFF'): Promise<PumpView> {
    return this.postJson('/api/pump', { state }, (raw) => PumpViewSchema.parse(raw));
  }

  async autoWaterState(): Promise<AutoWaterState> {
    return this.getJson('/api/auto-water/state',
      (raw) => AutoWaterStateSchema.parse(raw));
  }

  async permitJoin(duration_s: number): Promise<void> {
    await this.postJson('/api/zigbee/permit-join', { duration_s }, (x) => x);
  }

  async getDashboard(): Promise<DashboardView> {
    return this.getJson('/api/dashboard', (raw) => DashboardSchema.parse(raw));
  }

  async getConfig(): Promise<ConfigView> {
    return this.getJson('/api/config', (raw) => raw as ConfigView);
  }

  async setConfig(body: unknown): Promise<void> {
    await this.postJson('/api/config', body, (x) => x);
  }

  async history(ieee: string, kind: string, quantity: string, hours: number)
      : Promise<{ data: { ts_ms: number; value: number }[] }> {
    const url = `/api/history?ieee=${ieee}&kind=${kind}` +
                `&quantity=${quantity}&hours=${hours}`;
    return this.getJson(url, (x) => x as { data: { ts_ms: number; value: number }[] });
  }

  // Merge the Basic auth header (if credentials are stored) into request headers.
  private authHeaders(extra?: Record<string, string>): Record<string, string> {
    const h: Record<string, string> = { ...extra };
    const a = basicHeader();
    if (a) h.authorization = a;
    return h;
  }

  // Centralized failure path: a 401 clears credentials and flips the global
  // needsLogin signal so the whole app routes back to <Login> instead of
  // throwing into a blank screen.
  private fail(status: number): never {
    if (status === 401) {
      const hadCreds = getCreds() !== null;
      clearCreds();
      if (hadCreds) loginError.value = true;  // creds were rejected → wrong password
      needsLogin.value = true;
      throw new UnauthorizedError();
    }
    throw new Error(`HTTP ${status}`);
  }

  private async getJson<T>(path: string, parse: (raw: unknown) => T): Promise<T> {
    const r = await this.fetchImpl(path,
      { headers: this.authHeaders({ accept: 'application/json' }) });
    if (!r.ok) this.fail(r.status);
    return parse(await r.json());
  }

  private async postJson<T>(path: string, body: unknown,
                             parse: (raw: unknown) => T): Promise<T> {
    const r = await this.fetchImpl(path, {
      method: 'POST',
      headers: this.authHeaders({ 'content-type': 'application/json',
                                  accept: 'application/json' }),
      body: JSON.stringify(body),
    });
    if (!r.ok) this.fail(r.status);
    return parse(await r.json());
  }

  private async putJson(path: string, body: unknown): Promise<void> {
    const r = await this.fetchImpl(path, {
      method: 'PUT',
      headers: this.authHeaders({ 'content-type': 'application/json' }),
      body: JSON.stringify(body),
    });
    if (!r.ok) this.fail(r.status);
  }
}

export { UnauthorizedError, needsLogin, getCreds, setCreds, clearCreds } from './auth';
