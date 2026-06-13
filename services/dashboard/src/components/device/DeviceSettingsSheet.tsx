import { useEffect, useState } from "react";
import { toast } from "sonner";

import {
  Sheet,
  SheetContent,
  SheetDescription,
  SheetHeader,
  SheetTitle,
} from "@/components/ui/sheet";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";

import {
  useSensorCalibrationMutation,
  useDevicePatchMutation,
} from "@/hooks/useDeviceMutations";
import { useRevokeDeviceMutation } from "@/hooks/useRevokeDeviceMutation";
import type { Device } from "@/schemas/device";
import type { Sensor } from "@/schemas/sensor";

interface Props {
  open: boolean;
  onClose: () => void;
  device: Device;
  soilSensor?: Sensor;
}

function calNum(cal: Record<string, unknown> | null | undefined, key: string, fallback: number) {
  const v = cal?.[key];
  return typeof v === "number" ? v : fallback;
}

export function DeviceSettingsSheet({ open, onClose, device, soilSensor }: Props) {
  const [name, setName] = useState(device.friendly_name ?? "");
  const [rawDry, setRawDry] = useState(() => calNum(soilSensor?.calibration_json, "raw_dry", 249));
  const [rawWet, setRawWet] = useState(() => calNum(soilSensor?.calibration_json, "raw_wet", 489));
  const [confirmRevoke, setConfirmRevoke] = useState(false);

  useEffect(() => {
    if (open) {
      setName(device.friendly_name ?? "");
      setRawDry(calNum(soilSensor?.calibration_json, "raw_dry", 249));
      setRawWet(calNum(soilSensor?.calibration_json, "raw_wet", 489));
      setConfirmRevoke(false);
    }
  }, [open, device.friendly_name, soilSensor]);

  const patch = useDevicePatchMutation(device.device_id);
  const calibrate = useSensorCalibrationMutation(device.device_id);
  const revoke = useRevokeDeviceMutation();

  const liveMoisture = soilSensor?.last_value ?? null;

  function saveName() {
    patch.mutate(
      { friendly_name: name.trim() || null },
      {
        onSuccess: () => toast.success("Name saved"),
        onError: (e) => toast.error(`Save failed: ${(e as Error).message}`),
      },
    );
  }

  function saveCalibration() {
    if (!soilSensor) return;
    calibrate.mutate(
      {
        channelId: soilSensor.channel_id,
        kind: soilSensor.kind,
        calibration_json: { raw_dry: rawDry, raw_wet: rawWet },
      },
      {
        onSuccess: () => toast.success("Calibration saved"),
        onError: (e) => toast.error(`Save failed: ${(e as Error).message}`),
      },
    );
  }

  function doRevoke() {
    revoke.mutate(device.device_id, {
      onSuccess: () => {
        toast.success(`Revoked ${device.device_id}`);
        onClose();
      },
      onError: (e) => toast.error(`Revoke failed: ${(e as Error).message}`),
    });
  }

  return (
    <Sheet open={open} onOpenChange={(o) => !o && onClose()}>
      <SheetContent className="flex w-full flex-col gap-0 overflow-y-auto sm:max-w-[480px]">
        <SheetHeader>
          <SheetTitle className="font-display text-xl">Settings</SheetTitle>
          <SheetDescription className="font-mono text-xs">{device.device_id}</SheetDescription>
        </SheetHeader>

        <div className="mt-6 flex flex-col gap-4">
          {/* Device name */}
          <section className="glass flex flex-col gap-3 rounded-md p-4">
            <h3 className="font-display text-h2 font-semibold">Device</h3>
            <div className="flex flex-col gap-1.5">
              <Label htmlFor="friendly_name">Friendly name</Label>
              <Input
                id="friendly_name"
                value={name}
                placeholder={device.device_id}
                onChange={(e) => setName(e.target.value)}
              />
            </div>
            <Button
              size="sm"
              className="self-start"
              onClick={saveName}
              disabled={patch.isPending || name === (device.friendly_name ?? "")}
            >
              {patch.isPending ? "Saving…" : "Save name"}
            </Button>
          </section>

          {/* Soil calibration */}
          <section className="glass flex flex-col gap-3 rounded-md p-4">
            <div className="flex items-baseline justify-between gap-3">
              <h3 className="font-display text-h2 font-semibold">Soil calibration</h3>
              {liveMoisture != null && (
                <span className="num text-tiny text-muted-foreground">
                  live: <b className="text-foreground/80">{liveMoisture.toFixed(0)}%</b>
                </span>
              )}
            </div>
            {soilSensor ? (
              <>
                <p className="-mt-1 text-small text-muted-foreground">
                  Hold the sensor in dry air → save the dry point. Submerge it in water →
                  save the wet point. Anything between maps to 0–100 %.
                </p>
                <div
                  className="relative my-1 h-8 rounded-[8px] border border-border"
                  style={{
                    background:
                      "linear-gradient(90deg, hsl(40 68% 89%), hsl(86 48% 88%) 50%, hsl(194 35% 86%))",
                  }}
                >
                  {liveMoisture != null && (
                    <span
                      className="absolute top-1/2 h-3.5 w-3.5 -translate-x-1/2 -translate-y-1/2 rounded-full border-2 border-white bg-[hsl(var(--primary))] shadow"
                      style={{ left: `${Math.max(0, Math.min(100, liveMoisture))}%` }}
                    />
                  )}
                </div>
                <div className="flex gap-3">
                  <div className="flex flex-1 flex-col gap-1.5">
                    <Label htmlFor="raw_dry">Dry point</Label>
                    <Input
                      id="raw_dry"
                      type="number"
                      className="num"
                      value={rawDry}
                      min={0}
                      max={1023}
                      onChange={(e) => setRawDry(Number(e.target.value))}
                    />
                    <span className="text-tiny text-muted-foreground">raw ADC, sensor dry</span>
                  </div>
                  <div className="flex flex-1 flex-col gap-1.5">
                    <Label htmlFor="raw_wet">Wet point</Label>
                    <Input
                      id="raw_wet"
                      type="number"
                      className="num"
                      value={rawWet}
                      min={0}
                      max={1023}
                      onChange={(e) => setRawWet(Number(e.target.value))}
                    />
                    <span className="text-tiny text-muted-foreground">raw ADC, in water</span>
                  </div>
                </div>
                <Button
                  size="sm"
                  className="self-start"
                  onClick={saveCalibration}
                  disabled={calibrate.isPending}
                >
                  {calibrate.isPending ? "Saving…" : "Save calibration"}
                </Button>
              </>
            ) : (
              <p className="text-small text-muted-foreground">
                This device has no soil-moisture channel registered.
              </p>
            )}
          </section>

          {/* Device-side note */}
          <section className="glass flex flex-col gap-2 rounded-md p-4">
            <h3 className="font-display text-h2 font-semibold">Auto-water · Wi-Fi · MQTT</h3>
            <p className="text-small text-muted-foreground">
              These are configured on the coordinator itself (its local setup page), not
              through the hub. The hub only stores telemetry, calibration and queues pump
              commands.
            </p>
          </section>

          {/* System */}
          <section className="glass flex flex-col gap-3 rounded-md p-4">
            <h3 className="font-display text-h2 font-semibold">System</h3>
            <dl className="grid grid-cols-[1fr_auto] gap-x-4 gap-y-2 text-small">
              <dt className="text-muted-foreground">Device ID</dt>
              <dd className="font-mono">{device.device_id}</dd>
              <dt className="text-muted-foreground">Profile</dt>
              <dd>{device.profile_id}</dd>
              <dt className="text-muted-foreground">Firmware</dt>
              <dd className="num">v{device.fw_version ?? "—"}</dd>
              <dt className="text-muted-foreground">Sensors</dt>
              <dd className="num">{device.sensors_count}</dd>
            </dl>
          </section>

          {/* Danger zone */}
          <section
            className="flex flex-col gap-3 rounded-md p-4"
            style={{ background: "hsl(var(--destructive-bg))", border: "1px solid hsl(var(--destructive-border))" }}
          >
            <h3 className="font-display text-h2 font-semibold" style={{ color: "hsl(var(--danger-fg))" }}>
              Revoke credential
            </h3>
            <p className="text-small" style={{ color: "hsl(var(--danger-fg))" }}>
              The device stops sending data until it is re-paired with a fresh 6-digit
              code at the coordinator captive portal. This cannot be undone.
            </p>
            {confirmRevoke ? (
              <div className="flex gap-2">
                <Button
                  variant="destructive"
                  size="sm"
                  onClick={doRevoke}
                  disabled={revoke.isPending}
                >
                  {revoke.isPending ? "Revoking…" : "Confirm revoke"}
                </Button>
                <Button variant="outline" size="sm" onClick={() => setConfirmRevoke(false)}>
                  Cancel
                </Button>
              </div>
            ) : (
              <Button
                variant="destructive"
                size="sm"
                className="self-start"
                onClick={() => setConfirmRevoke(true)}
              >
                Revoke credential
              </Button>
            )}
          </section>
        </div>
      </SheetContent>
    </Sheet>
  );
}
