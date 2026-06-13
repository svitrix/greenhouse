import { GlassCard } from "./GlassCard";
import { StateBadge } from "./StateBadge";
import { Sparkline } from "./Sparkline";
import {
  STALE_THRESHOLD_S,
  secondsSince,
  type TileSpec,
} from "@/lib/sensor-state";
import type { Sensor } from "@/schemas/sensor";
import type { HistoryPoint } from "@/schemas/sensor";
import { cn } from "@/lib/utils";

interface Props {
  spec: TileSpec;
  sensor?: Sensor;
  history?: HistoryPoint[];
  nowMs: number;
}

export function SensorTile({ spec, sensor, history, nowMs }: Props) {
  const Icon = spec.icon;
  const value = sensor?.last_value ?? null;
  const staleS = secondsSince(sensor?.last_value_at, nowMs);
  const stale = staleS != null && staleS > STALE_THRESHOLD_S;
  const noData = value == null;
  const state = noData ? null : spec.stateFn(value);
  const hint = spec.kind === "soil_moist" && sensor?.calibration_json
    ? rawHint(sensor.calibration_json)
    : spec.hint;

  return (
    <GlassCard
      className={cn(
        "flex min-h-[168px] flex-col gap-3",
        stale && "opacity-65 saturate-50",
      )}
      data-tile={spec.kind}
    >
      <div className="flex items-center gap-2 text-muted-foreground">
        <span className="glass-2 inline-flex h-[26px] w-[26px] items-center justify-center rounded-[8px] text-[hsl(var(--primary))]">
          <Icon className="h-[18px] w-[18px]" strokeWidth={1.9} />
        </span>
        <span className="min-w-0 flex-1 truncate text-small">{spec.label}</span>
        {hint && <span className="num text-tiny text-muted-foreground/80">{hint}</span>}
      </div>

      <div className="num flex items-baseline gap-1 font-display text-display font-semibold leading-none text-foreground">
        {noData ? (
          <span className="text-muted-foreground">—</span>
        ) : (
          value.toFixed(spec.decimals)
        )}
        <span className="font-sans text-base font-medium text-muted-foreground">
          {spec.unit}
        </span>
      </div>

      <div className="mt-auto flex items-center justify-between gap-2">
        {state ? (
          <StateBadge tone={state.tone} label={state.label} />
        ) : (
          <StateBadge tone="nodata" label="no data" />
        )}
        {stale && <StateBadge tone="offline" label="offline" />}
      </div>

      <Sparkline
        points={(history ?? []).map((p) => p.v)}
        color={spec.accent}
        markLine={spec.markLine}
      />
    </GlassCard>
  );
}

function rawHint(cal: Record<string, unknown>): string {
  const dry = cal.raw_dry;
  const wet = cal.raw_wet;
  if (typeof dry === "number" && typeof wet === "number") {
    return `raw ${dry}–${wet}`;
  }
  return "";
}
