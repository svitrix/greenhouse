import { Droplet, Lock, TriangleAlert } from "lucide-react";
import { GlassCard } from "./GlassCard";
import type { Command } from "@/schemas/command";
import type { DeviceEvent } from "@/schemas/event";

const MAX_RUNTIME_S = 20;

type PumpState = "OFF" | "ON" | "LOCKED";

interface PumpView {
  state: PumpState;
  remainingS: number | null;
  lastRunMs: number | null;
  runsToday: number;
  lockReason: string | null;
}

export function derivePumpState(
  commands: Command[],
  events: DeviceEvent[],
  nowMs: number,
): PumpView {
  const cmds = [...commands].sort(
    (a, b) => Date.parse(b.created_at) - Date.parse(a.created_at),
  );
  const evts = [...events].sort((a, b) => Date.parse(b.ts) - Date.parse(a.ts));

  const watered = evts.filter((e) => e.kind === "watered");
  const lastRunMs = watered.length ? Date.parse(watered[0].ts) : null;
  const dayAgo = nowMs - 24 * 3600 * 1000;
  const runsToday = watered.filter((e) => Date.parse(e.ts) >= dayAgo).length;

  // locked: the most recent event is an aborted dry-run with no successful
  // watering after it — the firmware's lockout signal.
  const lastEvent = evts[0];
  if (lastEvent?.kind === "dry_run_aborted") {
    const reason =
      (lastEvent.payload?.reason as string | undefined) ?? "no_water_detected";
    return { state: "LOCKED", remainingS: null, lastRunMs, runsToday, lockReason: reason };
  }

  const latest = cmds[0];
  const running =
    latest?.command === "pump_on" &&
    (latest.status === "pending" || latest.status === "sent");
  if (running) {
    let remainingS: number | null = null;
    const durMs = latest.params_json?.duration_ms;
    if (typeof durMs === "number" && latest.claimed_at) {
      const elapsed = (nowMs - Date.parse(latest.claimed_at)) / 1000;
      remainingS = Math.max(0, Math.round(durMs / 1000 - elapsed));
    }
    return { state: "ON", remainingS, lastRunMs, runsToday, lockReason: null };
  }

  return { state: "OFF", remainingS: null, lastRunMs, runsToday, lockReason: null };
}

function ago(ms: number, nowMs: number): string {
  const s = Math.floor((nowMs - ms) / 1000);
  if (s < 60) return `${s}s ago`;
  if (s < 3600) return `${Math.floor(s / 60)}m ago`;
  if (s < 86400) return `${Math.floor(s / 3600)}h ago`;
  return `${Math.floor(s / 86400)}d ago`;
}

interface Props {
  pump: PumpView;
  channelLabel?: string;
  nowMs: number;
  pending: boolean;
  onStart: () => void;
  onStop: () => void;
}

export function PumpCard({ pump, channelLabel, nowMs, pending, onStart, onStop }: Props) {
  const stateText = pump.state === "ON" ? "Pouring" : pump.state === "LOCKED" ? "Locked" : "Idle";
  const caption =
    pump.state === "ON"
      ? "Soil getting a drink. Will stop automatically."
      : pump.state === "LOCKED"
        ? "Auto-water disabled until you check things."
        : `Tap to water for up to ${MAX_RUNTIME_S}s.`;

  const circ = 2 * Math.PI * 32;
  const off =
    pump.remainingS != null ? circ - (pump.remainingS / MAX_RUNTIME_S) * circ : circ;

  return (
    <GlassCard className="grid gap-4 sm:grid-cols-[1fr_auto] sm:items-center">
      <div className="flex min-w-0 flex-col gap-2">
        <div className="flex items-center gap-2 text-small text-muted-foreground">
          <span className="glass-2 inline-flex h-7 w-7 items-center justify-center rounded-[8px] text-[hsl(var(--primary))]">
            <Droplet className="h-5 w-5" strokeWidth={1.9} />
          </span>
          <span>Pump{channelLabel ? ` · ${channelLabel}` : ""}</span>
        </div>

        <div className="font-display text-[28px] font-semibold leading-[1.05] tracking-tight">
          {stateText}
        </div>
        <div className="text-small text-muted-foreground">{caption}</div>

        <div className="num mt-1 flex flex-wrap gap-x-4 gap-y-2 text-tiny text-muted-foreground">
          <span>
            {pump.lastRunMs ? (
              <>
                Last run <b className="font-semibold text-foreground/80">{ago(pump.lastRunMs, nowMs)}</b>
              </>
            ) : (
              "No runs yet"
            )}
          </span>
          <span>
            Today: <b className="font-semibold text-foreground/80">{pump.runsToday} runs</b>
          </span>
        </div>

        {pump.state === "LOCKED" && (
          <div
            data-tone="alert"
            className="state-badge mt-2 !items-start !gap-3 !rounded-[10px] !px-3 !py-3 text-small before:hidden"
          >
            <TriangleAlert className="h-5 w-5 shrink-0" />
            <div>
              <b className="mb-0.5 block font-semibold">No water detected.</b>
              Soil stayed dry through auto-water attempts ({pump.lockReason}). Refill
              the reservoir or check the line, then power-cycle to clear the lock.
            </div>
          </div>
        )}
      </div>

      <div className="relative flex items-center gap-[18px]">
        {pump.state === "LOCKED" ? (
          <button
            disabled
            className="inline-flex min-h-[56px] min-w-[132px] cursor-not-allowed items-center justify-center gap-2 rounded-[18px] bg-muted px-[22px] font-display text-lg font-semibold text-muted-foreground ring-1 ring-inset ring-border"
          >
            <Lock className="h-[18px] w-[18px]" /> Locked
          </button>
        ) : pump.state === "ON" ? (
          <button
            onClick={onStop}
            disabled={pending}
            className="inline-flex min-h-[56px] min-w-[132px] items-center justify-center rounded-[18px] bg-lime px-[22px] font-display text-lg font-semibold text-[hsl(var(--lime-foreground))] shadow-[0_3px_0_hsl(var(--lime-deep)),0_8px_18px_-10px_rgba(120,140,20,0.5)] transition-transform active:translate-y-0.5 disabled:opacity-60"
          >
            <span className="mr-2.5 h-2.5 w-2.5 animate-gh-blink rounded-full bg-[hsl(var(--lime-foreground))]" />
            Stop now
          </button>
        ) : (
          <button
            onClick={onStart}
            disabled={pending}
            className="inline-flex min-h-[56px] min-w-[132px] items-center justify-center rounded-[18px] bg-primary px-[22px] font-display text-lg font-semibold text-primary-foreground shadow-[0_3px_0_hsl(var(--primary-hover)),0_8px_18px_-10px_rgba(50,80,40,0.5)] transition-transform active:translate-y-0.5 disabled:opacity-60"
          >
            <span className="mr-2.5 h-2.5 w-2.5 rounded-full bg-white" />
            {pending ? "Starting…" : "Start watering"}
          </button>
        )}

        {pump.state === "ON" && pump.remainingS != null && (
          <div className="relative inline-flex h-[84px] w-[84px] items-center justify-center">
            <svg viewBox="0 0 72 72" className="absolute inset-0 -rotate-90">
              <circle cx="36" cy="36" r="32" fill="none" stroke="hsl(var(--border))" strokeWidth={6} />
              <circle
                cx="36"
                cy="36"
                r="32"
                fill="none"
                stroke="hsl(var(--k-soil))"
                strokeWidth={6}
                strokeLinecap="round"
                strokeDasharray={circ}
                strokeDashoffset={off}
                className="transition-[stroke-dashoffset] duration-700 ease-linear"
              />
            </svg>
            <span className="num font-display text-2xl font-semibold tabular-nums">
              {pump.remainingS}
            </span>
          </div>
        )}
      </div>
    </GlassCard>
  );
}
