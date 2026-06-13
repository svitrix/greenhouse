import { useQuery } from "@tanstack/react-query";
import { ApiError, apiFetch } from "@/lib/api";
import { deviceSchema, type Device } from "@/schemas/device";
import { sensorsListSchema, type Sensor, historyListSchema, type HistoryPoint } from "@/schemas/sensor";
import { commandsListSchema, type Command } from "@/schemas/command";
import { eventsListSchema, type DeviceEvent } from "@/schemas/event";

export const deviceKeys = {
  detail: (id: string) => ["device", id] as const,
  sensors: (id: string) => ["device", id, "sensors"] as const,
  commands: (id: string) => ["device", id, "commands"] as const,
  events: (id: string) => ["device", id, "events"] as const,
  history: (id: string, ch: number, kind: string, hours: number) =>
    ["device", id, "history", ch, kind, hours] as const,
};

export function useDeviceQuery(deviceId: string) {
  return useQuery({
    queryKey: deviceKeys.detail(deviceId),
    queryFn: async (): Promise<Device> =>
      deviceSchema.parse(await apiFetch<unknown>(`/api/devices/${deviceId}`)),
    refetchInterval: 30_000,
  });
}

export function useDeviceSensorsQuery(deviceId: string) {
  return useQuery({
    queryKey: deviceKeys.sensors(deviceId),
    queryFn: async (): Promise<Sensor[]> =>
      sensorsListSchema.parse(
        await apiFetch<unknown>(
          `/api/sensors?device_id=${encodeURIComponent(deviceId)}`,
        ),
      ),
    refetchInterval: 15_000,
  });
}

export function useDeviceCommandsQuery(deviceId: string) {
  return useQuery({
    queryKey: deviceKeys.commands(deviceId),
    queryFn: async (): Promise<Command[]> =>
      commandsListSchema.parse(
        await apiFetch<unknown>(`/api/devices/${deviceId}/commands`),
      ),
    refetchInterval: 10_000,
  });
}

/** The hub events endpoint filters by kind/since only, so we pull the recent
 *  window and narrow to this device client-side. */
export function useDeviceEventsQuery(deviceId: string) {
  return useQuery({
    queryKey: deviceKeys.events(deviceId),
    queryFn: async (): Promise<DeviceEvent[]> => {
      const all = eventsListSchema.parse(
        await apiFetch<unknown>(`/api/events?limit=200`),
      );
      return all.filter((e) => e.device_id === deviceId);
    },
    refetchInterval: 30_000,
  });
}

/** Sparkline history. The hub does not expose a timeseries endpoint yet, so we
 *  treat any non-200 as "no history" and let the tile render without a spark. */
export function useSensorHistoryQuery(
  deviceId: string,
  channelId: number | undefined,
  kind: string,
  hours = 24,
) {
  return useQuery({
    queryKey: deviceKeys.history(deviceId, channelId ?? -1, kind, hours),
    enabled: channelId != null,
    staleTime: 60_000,
    queryFn: async (): Promise<HistoryPoint[]> => {
      try {
        const raw = await apiFetch<unknown>(
          `/api/sensors/${deviceId}/${channelId}/${kind}/history?hours=${hours}`,
        );
        return historyListSchema.parse(raw);
      } catch (e) {
        if (e instanceof ApiError) return [];
        throw e;
      }
    },
  });
}
