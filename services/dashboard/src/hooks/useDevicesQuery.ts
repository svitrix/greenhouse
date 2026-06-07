import { useQuery } from "@tanstack/react-query";
import { apiFetch } from "@/lib/api";
import { type Device, devicesListSchema } from "@/schemas/device";

export const DEVICES_QUERY_KEY = ["devices"] as const;

export function useDevicesQuery() {
  return useQuery({
    queryKey: DEVICES_QUERY_KEY,
    queryFn: async (): Promise<Device[]> => {
      const raw = await apiFetch<unknown>("/api/devices");
      return devicesListSchema.parse(raw);
    },
    staleTime: 30_000,
    refetchInterval: 30_000,
    refetchOnWindowFocus: true,
  });
}
