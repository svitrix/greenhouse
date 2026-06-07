import { useMutation, useQueryClient } from "@tanstack/react-query";
import { apiFetch } from "@/lib/api";
import { DEVICES_QUERY_KEY } from "./useDevicesQuery";

export function useRevokeDeviceMutation() {
  const qc = useQueryClient();
  return useMutation({
    mutationFn: async (deviceId: string) => {
      await apiFetch<void>(`/api/devices/${deviceId}/revoke`, {
        method: "POST",
      });
    },
    onSuccess: () => {
      qc.invalidateQueries({ queryKey: DEVICES_QUERY_KEY });
    },
  });
}
