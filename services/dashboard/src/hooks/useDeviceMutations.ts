import { useMutation, useQueryClient } from "@tanstack/react-query";
import { apiFetch } from "@/lib/api";
import { commandSchema, type CommandKind } from "@/schemas/command";
import { deviceSchema } from "@/schemas/device";
import { sensorSchema } from "@/schemas/sensor";
import { deviceKeys } from "./useDeviceDetail";
import { DEVICES_QUERY_KEY } from "./useDevicesQuery";

/** Queue a pump_on / pump_off command for the coordinator to pick up. */
export function usePumpCommandMutation(deviceId: string) {
  const qc = useQueryClient();
  return useMutation({
    mutationFn: async (input: {
      command: CommandKind;
      params?: Record<string, unknown>;
    }) =>
      commandSchema.parse(
        await apiFetch<unknown>(`/api/devices/${deviceId}/commands`, {
          method: "POST",
          body: JSON.stringify(input),
        }),
      ),
    onSuccess: () =>
      qc.invalidateQueries({ queryKey: deviceKeys.commands(deviceId) }),
  });
}

/** Patch a sensor's soil calibration (raw_dry / raw_wet) etc. */
export function useSensorCalibrationMutation(deviceId: string) {
  const qc = useQueryClient();
  return useMutation({
    mutationFn: async (input: {
      channelId: number;
      kind: string;
      calibration_json: Record<string, unknown>;
    }) =>
      sensorSchema.parse(
        await apiFetch<unknown>(
          `/api/sensors/${deviceId}/${input.channelId}/${input.kind}`,
          {
            method: "PATCH",
            body: JSON.stringify({ calibration_json: input.calibration_json }),
          },
        ),
      ),
    onSuccess: () =>
      qc.invalidateQueries({ queryKey: deviceKeys.sensors(deviceId) }),
  });
}

/** Edit device friendly_name / location_id. */
export function useDevicePatchMutation(deviceId: string) {
  const qc = useQueryClient();
  return useMutation({
    mutationFn: async (input: {
      friendly_name?: string | null;
      location_id?: string | null;
    }) =>
      deviceSchema.parse(
        await apiFetch<unknown>(`/api/devices/${deviceId}`, {
          method: "PATCH",
          body: JSON.stringify(input),
        }),
      ),
    onSuccess: () => {
      qc.invalidateQueries({ queryKey: deviceKeys.detail(deviceId) });
      qc.invalidateQueries({ queryKey: DEVICES_QUERY_KEY });
    },
  });
}
