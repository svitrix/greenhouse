import { useQuery } from "@tanstack/react-query";
import { apiFetch } from "@/lib/api";
import { eventsListSchema, type DeviceEvent } from "@/schemas/event";

export const EVENTS_QUERY_KEY = ["events"] as const;

/** Recent events across all devices — feeds the dashboard activity panel. */
export function useEventsQuery(limit = 30) {
  return useQuery({
    queryKey: [...EVENTS_QUERY_KEY, limit] as const,
    queryFn: async (): Promise<DeviceEvent[]> =>
      eventsListSchema.parse(await apiFetch<unknown>(`/api/events?limit=${limit}`)),
    refetchInterval: 30_000,
  });
}
