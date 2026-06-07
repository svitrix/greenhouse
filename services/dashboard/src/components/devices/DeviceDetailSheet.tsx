import { formatDistanceToNow, parseISO } from "date-fns";
import {
  Sheet,
  SheetContent,
  SheetDescription,
  SheetHeader,
  SheetTitle,
} from "@/components/ui/sheet";
import { Badge } from "@/components/ui/badge";
import type { Device } from "@/schemas/device";

interface Props {
  device: Device | null;
  onClose: () => void;
}

export function DeviceDetailSheet({ device, onClose }: Props) {
  return (
    <Sheet open={device !== null} onOpenChange={(open) => !open && onClose()}>
      <SheetContent>
        {device && (
          <>
            <SheetHeader>
              <SheetTitle className="font-mono text-base">
                {device.device_id}
              </SheetTitle>
              <SheetDescription>
                {device.friendly_name ?? "no friendly name yet (D-3b)"}
              </SheetDescription>
            </SheetHeader>
            <dl className="mt-6 space-y-3 text-sm">
              <Row label="Profile">
                <Badge variant="outline">{device.profile_id}</Badge>
              </Row>
              <Row label="Firmware">{device.fw_version ?? "—"}</Row>
              <Row label="Location">
                {device.location_id ?? (
                  <em className="text-muted-foreground">unassigned</em>
                )}
              </Row>
              <Row label="Sensors">{device.sensors_count}</Row>
              <Row label="Status">
                {device.online ? "online" : "offline"}
              </Row>
              <Row label="Last seen">
                {formatDistanceToNow(parseISO(device.last_seen_at), {
                  addSuffix: true,
                })}
              </Row>
            </dl>
            <p className="mt-6 text-xs text-muted-foreground">
              Edit friendly_name / location_id → coming in D-3b.
            </p>
          </>
        )}
      </SheetContent>
    </Sheet>
  );
}

function Row({ label, children }: { label: string; children: React.ReactNode }) {
  return (
    <div className="flex items-center justify-between">
      <dt className="text-muted-foreground">{label}</dt>
      <dd>{children}</dd>
    </div>
  );
}
