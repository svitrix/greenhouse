import { z } from 'zod';

export const ReadingSchema = z.object({
  kind: z.enum(['air', 'soil1', 'battery']),
  quantity: z.enum([
    'temp_c', 'humidity_pct', 'moisture_pct',
    'soil_temp_c', 'pct', 'voltage_v',
  ]),
  value: z.number(),
  unit: z.string(),
  age_s: z.number().nonnegative(),
});

export const NodeViewSchema = z.object({
  ieee: z.string().regex(/^[0-9A-Fa-f]{16}$/),
  short_addr: z.string().regex(/^0x[0-9A-Fa-f]{4}$/),
  alias: z.string().nullable(),
  online: z.boolean(),
  last_seen_s: z.number(),
  rssi_dbm: z.number().int(),
  proto_version: z.number().int(),
  proto_version_mismatch: z.boolean(),
  present_mask: z.string(),
  readings: z.array(ReadingSchema),
});

export const NodesResponseSchema = z.object({
  ts_ms: z.number(),
  nodes: z.array(NodeViewSchema),
});

export const PumpViewSchema = z.object({
  state: z.enum(['ON', 'OFF', 'LOCKED']),
  remaining_s: z.number(),
  last_run_ms: z.number(),
  lockout_reason: z.string().optional(),
});

export const AutoWaterStateSchema = z.object({
  avg_moisture_pct: z.number().nullable(),
  fresh_sources: z.array(z.string()),
  stale_sources: z.array(z.string()),
  last_decision_ms: z.number(),
  last_decision: z.string(),
});

// Combined dashboard payload pushed over SSE (/api/events) and served by
// GET /api/dashboard. Composed from the per-endpoint schemas above.
export const DashboardSchema = z.object({
  ts_ms: z.number(),
  nodes: z.array(NodeViewSchema),
  pump: PumpViewSchema,
  auto: AutoWaterStateSchema,
});
