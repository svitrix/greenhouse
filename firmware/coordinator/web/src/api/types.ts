export type Kind     = 'air' | 'soil1' | 'battery';
export type Quantity = 'temp_c' | 'humidity_pct' | 'moisture_pct'
                      | 'soil_temp_c' | 'pct' | 'voltage_v';

export interface Reading {
  kind: Kind;
  quantity: Quantity;
  value: number;
  unit: string;
  age_s: number;
}

export interface NodeView {
  ieee: string;
  short_addr: string;
  alias: string | null;
  online: boolean;
  last_seen_s: number;
  rssi_dbm: number;
  proto_version: number;
  proto_version_mismatch: boolean;
  present_mask: string;
  readings: Reading[];
}

export interface NodesResponse {
  ts_ms: number;
  nodes: NodeView[];
}

export interface PumpView {
  state: 'ON' | 'OFF' | 'LOCKED';
  remaining_s: number;
  last_run_ms: number;
  lockout_reason?: string;
}

export interface AutoWaterState {
  avg_moisture_pct: number | null;
  fresh_sources: string[];
  stale_sources: string[];
  last_decision_ms: number;
  last_decision: string;
}

export interface DashboardView {
  ts_ms: number;
  nodes: NodeView[];
  pump: PumpView;
  auto: AutoWaterState;
}

export interface ConfigView {
  auto_water: {
    enabled: boolean;
    trigger_below_pct: number;
    min_interval_min: number;
    duration_s: number;
    min_fresh_sources: number;
    stale_threshold_s: number;
  };
  mqtt: { host: string; port: number; user: string; password_set: boolean; };
  wifi: { ssid: string; };
}
