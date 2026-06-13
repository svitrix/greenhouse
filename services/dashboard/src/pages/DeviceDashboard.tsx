import { useEffect, useState } from "react";
import { Link, useParams } from "react-router-dom";
import { toast } from "sonner";
import { WifiOff } from "lucide-react";

import { DeviceHeader } from "@/components/device/DeviceHeader";
import { SensorTile } from "@/components/device/SensorTile";
import { PumpCard, derivePumpState } from "@/components/device/PumpCard";
import { DeviceSettingsSheet } from "@/components/device/DeviceSettingsSheet";
import { GlassCard } from "@/components/device/GlassCard";
import { Button } from "@/components/ui/button";

import {
  useDeviceQuery,
  useDeviceSensorsQuery,
  useDeviceCommandsQuery,
  useDeviceEventsQuery,
  useSensorHistoryQuery,
} from "@/hooks/useDeviceDetail";
import { usePumpCommandMutation } from "@/hooks/useDeviceMutations";
import { TILE_SPECS, type TileKind } from "@/lib/sensor-state";
import { formatDistanceToNow, parseISO } from "date-fns";

const DEFAULT_DURATION_S = 15;

function useNow(intervalMs = 1000): number {
  const [now, setNow] = useState(() => Date.now());
  useEffect(() => {
    const id = setInterval(() => setNow(Date.now()), intervalMs);
    return () => clearInterval(id);
  }, [intervalMs]);
  return now;
}

export default function DeviceDashboard() {
  const { deviceId = "" } = useParams();
  const nowMs = useNow();
  const [settingsOpen, setSettingsOpen] = useState(false);

  const device = useDeviceQuery(deviceId);
  const sensors = useDeviceSensorsQuery(deviceId);
  const commands = useDeviceCommandsQuery(deviceId);
  const events = useDeviceEventsQuery(deviceId);
  const pumpMut = usePumpCommandMutation(deviceId);

  const byKind = (kind: TileKind) =>
    sensors.data?.find((s) => s.kind === kind);

  // 4 fixed tiles → 4 fixed history hooks (rules-of-hooks safe).
  const histAirTemp = useSensorHistoryQuery(deviceId, byKind("air_temp")?.channel_id, "air_temp");
  const histAirHum = useSensorHistoryQuery(deviceId, byKind("air_humidity")?.channel_id, "air_humidity");
  const histSoilMoist = useSensorHistoryQuery(deviceId, byKind("soil_moist")?.channel_id, "soil_moist");
  const histSoilTemp = useSensorHistoryQuery(deviceId, byKind("soil_temp")?.channel_id, "soil_temp");
  const historyByKind: Record<TileKind, typeof histAirTemp.data> = {
    air_temp: histAirTemp.data,
    air_humidity: histAirHum.data,
    soil_moist: histSoilMoist.data,
    soil_temp: histSoilTemp.data,
  };

  if (device.isError) {
    return (
      <div className="mx-auto max-w-[860px]">
        <GlassCard className="flex flex-col items-center gap-3 p-6 text-center">
          <span
            className="inline-flex h-[76px] w-[76px] items-center justify-center rounded-full"
            style={{ background: "hsl(var(--danger-bg))", color: "hsl(var(--danger-fg))" }}
          >
            <WifiOff className="h-9 w-9" />
          </span>
          <div className="font-display text-xl font-bold">Can't reach this device.</div>
          <p className="max-w-[36ch] text-small text-muted-foreground">
            The hub has no record for <span className="font-mono">{deviceId}</span>, or
            it is temporarily unreachable.
          </p>
          <Button asChild variant="outline" className="mt-1">
            <Link to="/devices">Back to devices</Link>
          </Button>
        </GlassCard>
      </div>
    );
  }

  const d = device.data;
  const pump = derivePumpState(commands.data ?? [], events.data ?? [], nowMs);
  const soilSensor = byKind("soil_moist");

  function startPump() {
    pumpMut.mutate(
      { command: "pump_on", params: { duration_ms: DEFAULT_DURATION_S * 1000 } },
      {
        onSuccess: () => toast.success("Watering command queued"),
        onError: (e) => toast.error(`Command failed: ${(e as Error).message}`),
      },
    );
  }
  function stopPump() {
    pumpMut.mutate(
      { command: "pump_off" },
      {
        onSuccess: () => toast.success("Stop command queued"),
        onError: (e) => toast.error(`Command failed: ${(e as Error).message}`),
      },
    );
  }

  return (
    <div className="mx-auto max-w-[860px]">
      <DeviceHeader
        name={d?.friendly_name || deviceId}
        subtitle={`${deviceId} · ${d?.profile_id ?? "—"}`}
        online={d?.online ?? false}
        onOpenSettings={() => setSettingsOpen(true)}
      />

      <div className="grid grid-cols-1 gap-3 sm:grid-cols-2 xl:grid-cols-4 [&>*:last-child]:sm:col-span-2 [&>*:last-child]:xl:col-span-4">
        {TILE_SPECS.map((spec) => (
          <SensorTile
            key={spec.kind}
            spec={spec}
            sensor={byKind(spec.kind)}
            history={historyByKind[spec.kind]}
            nowMs={nowMs}
          />
        ))}
        <PumpCard
          pump={pump}
          nowMs={nowMs}
          pending={pumpMut.isPending}
          onStart={startPump}
          onStop={stopPump}
        />
      </div>

      <div className="num mt-6 flex flex-wrap items-center justify-between gap-2 rounded-md px-4 py-3 text-tiny text-muted-foreground glass-2">
        <span>
          Firmware <b className="font-medium text-foreground/80">v{d?.fw_version ?? "—"}</b>
        </span>
        <span className="text-border">·</span>
        <span>
          Profile <b className="font-medium text-foreground/80">{d?.profile_id ?? "—"}</b>
        </span>
        <span className="text-border">·</span>
        <span>
          Sensors <b className="font-medium text-foreground/80">{d?.sensors_count ?? 0}</b>
        </span>
        <span className="text-border">·</span>
        <span>
          Last seen{" "}
          <b className="font-medium text-foreground/80">
            {d?.last_seen_at
              ? formatDistanceToNow(parseISO(d.last_seen_at), { addSuffix: true })
              : "—"}
          </b>
        </span>
      </div>

      {d && (
        <DeviceSettingsSheet
          open={settingsOpen}
          onClose={() => setSettingsOpen(false)}
          device={d}
          soilSensor={soilSensor}
        />
      )}
    </div>
  );
}
