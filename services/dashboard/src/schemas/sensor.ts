import { z } from "zod";

export const sensorSchema = z.object({
  device_id: z.string(),
  channel_id: z.number().int(),
  kind: z.string(),
  unit: z.string(),
  friendly_name: z.string().nullable(),
  plant_group_id: z.string().nullable(),
  calibration_json: z.record(z.unknown()).nullable(),
  last_value: z.number().nullable(),
  last_value_at: z.string().nullable(),
  created_at: z.string(),
});

export type Sensor = z.infer<typeof sensorSchema>;

export const sensorsListSchema = z.array(sensorSchema);

/** One sparkline point as returned by the (optional) history endpoint. */
export const historyPointSchema = z.object({
  t_ms: z.number(),
  v: z.number(),
});

export type HistoryPoint = z.infer<typeof historyPointSchema>;

export const historyListSchema = z.array(historyPointSchema);
