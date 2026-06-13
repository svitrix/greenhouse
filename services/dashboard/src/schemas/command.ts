import { z } from "zod";

export const commandKindSchema = z.enum(["pump_on", "pump_off"]);
export type CommandKind = z.infer<typeof commandKindSchema>;

export const commandSchema = z.object({
  id: z.string(),
  device_id: z.string(),
  command: z.string(),
  params_json: z.record(z.unknown()).nullable(),
  status: z.string(), // pending | sent | acked | failed | expired
  created_by: z.string().nullable(),
  created_at: z.string(),
  claimed_at: z.string().nullable(),
  acked_at: z.string().nullable(),
  result_json: z.record(z.unknown()).nullable(),
});

export type Command = z.infer<typeof commandSchema>;

export const commandsListSchema = z.array(commandSchema);
