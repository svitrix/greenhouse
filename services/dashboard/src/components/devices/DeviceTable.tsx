import { formatDistanceToNow, parseISO } from "date-fns";
import { MoreHorizontal } from "lucide-react";

import {
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from "@/components/ui/table";
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuTrigger,
} from "@/components/ui/dropdown-menu";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";

import type { Device } from "@/schemas/device";
import { OnlineBadge } from "./OnlineBadge";

interface Props {
  devices: Device[];
  onRevoke: (deviceId: string) => void;
  onShowDetail: (device: Device) => void;
  onOpenDevice?: (deviceId: string) => void;
}

export function DeviceTable({
  devices,
  onRevoke,
  onShowDetail,
  onOpenDevice,
}: Props) {
  return (
    <Table>
      <TableHeader>
        <TableRow>
          <TableHead>Device ID</TableHead>
          <TableHead>Friendly name</TableHead>
          <TableHead>Profile</TableHead>
          <TableHead className="text-right">Sensors</TableHead>
          <TableHead>Status</TableHead>
          <TableHead>Last seen</TableHead>
          <TableHead className="w-12"></TableHead>
        </TableRow>
      </TableHeader>
      <TableBody>
        {devices.length === 0 && (
          <TableRow>
            <TableCell
              colSpan={7}
              className="text-center text-muted-foreground"
            >
              No devices yet — pair one to get started.
            </TableCell>
          </TableRow>
        )}
        {devices.map((d) => (
          <TableRow key={d.device_id}>
            <TableCell className="font-mono text-xs">
              {onOpenDevice ? (
                <button
                  onClick={() => onOpenDevice(d.device_id)}
                  className="font-medium text-[hsl(var(--primary))] underline-offset-2 hover:underline"
                >
                  {d.device_id}
                </button>
              ) : (
                d.device_id
              )}
            </TableCell>
            <TableCell>
              {d.friendly_name ?? (
                <span className="italic text-muted-foreground">—</span>
              )}
            </TableCell>
            <TableCell>
              <Badge variant="outline">{d.profile_id}</Badge>
            </TableCell>
            <TableCell className="text-right tabular-nums">
              {d.sensors_count}
            </TableCell>
            <TableCell>
              <OnlineBadge online={d.online} />
            </TableCell>
            <TableCell>
              {formatDistanceToNow(parseISO(d.last_seen_at), {
                addSuffix: true,
              })}
            </TableCell>
            <TableCell>
              <DropdownMenu>
                <DropdownMenuTrigger asChild>
                  <Button variant="ghost" size="icon" aria-label="row actions">
                    <MoreHorizontal className="h-4 w-4" />
                  </Button>
                </DropdownMenuTrigger>
                <DropdownMenuContent align="end">
                  {onOpenDevice && (
                    <DropdownMenuItem onClick={() => onOpenDevice(d.device_id)}>
                      Open dashboard
                    </DropdownMenuItem>
                  )}
                  <DropdownMenuItem onClick={() => onShowDetail(d)}>
                    Quick details
                  </DropdownMenuItem>
                  <DropdownMenuItem
                    className="text-destructive"
                    onClick={() => onRevoke(d.device_id)}
                  >
                    Revoke credential
                  </DropdownMenuItem>
                </DropdownMenuContent>
              </DropdownMenu>
            </TableCell>
          </TableRow>
        ))}
      </TableBody>
    </Table>
  );
}
