import { z } from "zod";

export const eventKindSchema = z.enum([
  "watered",
  "dry_run_aborted",
  "sensor_offline",
  "provisioned",
]);
export type EventKind = z.infer<typeof eventKindSchema>;

export const eventSchema = z.object({
  ts: z.string(),
  device_id: z.string(),
  device_name: z.string().nullable(),
  kind: z.string(),
  payload: z.record(z.unknown()).nullable(),
});

export type DeviceEvent = z.infer<typeof eventSchema>;

export const eventsListSchema = z.array(eventSchema);
