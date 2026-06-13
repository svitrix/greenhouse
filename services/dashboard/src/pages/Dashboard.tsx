import { Link } from "react-router-dom";
import { formatDistanceToNow, parseISO } from "date-fns";
import { Cpu, Droplet, Radio, Sprout, TriangleAlert, Wifi } from "lucide-react";

import { GlassCard } from "@/components/device/GlassCard";
import { OnlineBadge } from "@/components/devices/OnlineBadge";
import { useDevicesQuery } from "@/hooks/useDevicesQuery";
import { useEventsQuery } from "@/hooks/useEventsQuery";
import type { DeviceEvent } from "@/schemas/event";
import { cn } from "@/lib/utils";

const EVENT_META: Record<
  string,
  { icon: typeof Droplet; label: string; tone: string }
> = {
  watered: { icon: Droplet, label: "Watered", tone: "hsl(var(--info))" },
  dry_run_aborted: { icon: TriangleAlert, label: "Dry-run aborted", tone: "hsl(var(--danger))" },
  sensor_offline: { icon: Radio, label: "Sensor offline", tone: "hsl(var(--warning))" },
  provisioned: { icon: Sprout, label: "Provisioned", tone: "hsl(var(--primary))" },
};

export default function Dashboard() {
  const devices = useDevicesQuery();
  const events = useEventsQuery(20);

  const total = devices.data?.length ?? 0;
  const online = devices.data?.filter((d) => d.online).length ?? 0;
  const sensors = devices.data?.reduce((n, d) => n + d.sensors_count, 0) ?? 0;

  return (
    <div className="space-y-6">
      <div>
        <h1 className="font-display text-h1 font-semibold">Greenhouse overview</h1>
        <p className="text-small text-muted-foreground">
          {total === 0 ? "No devices paired yet." : `${online} of ${total} devices online.`}
        </p>
      </div>

      <div className="grid grid-cols-2 gap-3 sm:grid-cols-4">
        <Stat icon={Cpu} label="Devices" value={total} />
        <Stat icon={Wifi} label="Online" value={online} accent="hsl(var(--primary))" />
        <Stat icon={Sprout} label="Sensors" value={sensors} />
        <Stat icon={Droplet} label="Events (24h)" value={events.data?.length ?? 0} />
      </div>

      <div className="grid grid-cols-1 gap-6 lg:grid-cols-[1.5fr_1fr]">
        {/* Devices grid */}
        <section className="space-y-3">
          <h2 className="font-display text-h2 font-semibold">Devices</h2>
          {devices.isLoading && (
            <p className="text-small text-muted-foreground">Loading…</p>
          )}
          {devices.data && devices.data.length === 0 && (
            <GlassCard className="text-small text-muted-foreground">
              No devices yet — pair one to get started.
            </GlassCard>
          )}
          <div className="grid grid-cols-1 gap-3 sm:grid-cols-2">
            {devices.data?.map((d) => (
              <Link key={d.device_id} to={`/devices/${d.device_id}`}>
                <GlassCard className="flex flex-col gap-3 transition-shadow hover:shadow-pop">
                  <div className="flex items-start justify-between gap-2">
                    <div className="min-w-0">
                      <div className="truncate font-display text-h2 font-semibold">
                        {d.friendly_name || d.device_id}
                      </div>
                      <div className="num truncate text-tiny text-muted-foreground">
                        {d.device_id}
                      </div>
                    </div>
                    <OnlineBadge online={d.online} />
                  </div>
                  <div className="num flex items-center gap-4 text-tiny text-muted-foreground">
                    <span className="inline-flex items-center gap-1">
                      <Sprout className="h-3.5 w-3.5" /> {d.sensors_count} sensors
                    </span>
                    <span>
                      seen {formatDistanceToNow(parseISO(d.last_seen_at), { addSuffix: true })}
                    </span>
                  </div>
                </GlassCard>
              </Link>
            ))}
          </div>
        </section>

        {/* Recent activity */}
        <section className="space-y-3">
          <h2 className="font-display text-h2 font-semibold">Recent activity</h2>
          <GlassCard className="divide-y divide-border/60 p-0">
            {events.isLoading && (
              <p className="p-4 text-small text-muted-foreground">Loading…</p>
            )}
            {events.data && events.data.length === 0 && (
              <p className="p-4 text-small text-muted-foreground">No events yet.</p>
            )}
            {events.data?.map((e, i) => (
              <EventRow key={`${e.ts}-${i}`} event={e} />
            ))}
          </GlassCard>
        </section>
      </div>
    </div>
  );
}

function Stat({
  icon: Icon,
  label,
  value,
  accent,
}: {
  icon: typeof Droplet;
  label: string;
  value: number;
  accent?: string;
}) {
  return (
    <GlassCard className="flex flex-col gap-1 p-4">
      <span
        className="glass-2 inline-flex h-8 w-8 items-center justify-center rounded-[10px]"
        style={{ color: accent ?? "hsl(var(--primary))" }}
      >
        <Icon className="h-[18px] w-[18px]" strokeWidth={1.9} />
      </span>
      <span className="num font-display text-h1 font-semibold leading-tight">{value}</span>
      <span className="text-tiny text-muted-foreground">{label}</span>
    </GlassCard>
  );
}

function EventRow({ event }: { event: DeviceEvent }) {
  const meta = EVENT_META[event.kind] ?? {
    icon: Radio,
    label: event.kind,
    tone: "hsl(var(--muted-foreground))",
  };
  const Icon = meta.icon;
  return (
    <div className="flex items-center gap-3 p-3">
      <span
        className={cn("inline-flex h-8 w-8 shrink-0 items-center justify-center rounded-full")}
        style={{ background: "hsl(var(--accent))", color: meta.tone }}
      >
        <Icon className="h-4 w-4" />
      </span>
      <div className="min-w-0 flex-1">
        <div className="truncate text-small font-medium">{meta.label}</div>
        <div className="num truncate text-tiny text-muted-foreground">
          {event.device_name || event.device_id}
        </div>
      </div>
      <span className="num shrink-0 text-tiny text-muted-foreground">
        {formatDistanceToNow(parseISO(event.ts), { addSuffix: true })}
      </span>
    </div>
  );
}
