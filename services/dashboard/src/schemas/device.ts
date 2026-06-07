import { z } from "zod";

export const deviceSchema = z.object({
  device_id: z.string(),
  friendly_name: z.string().nullable(),
  fw_version: z.string().nullable(),
  profile_id: z.string(),
  location_id: z.string().nullable(),
  last_seen_at: z.string(), // ISO timestamp
  sensors_count: z.number().int(),
  online: z.boolean(),
});

export type Device = z.infer<typeof deviceSchema>;

export const devicesListSchema = z.array(deviceSchema);
