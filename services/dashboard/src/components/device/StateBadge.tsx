import { cn } from "@/lib/utils";
import type { StateTone } from "@/lib/sensor-state";

interface Props {
  tone: StateTone;
  label: string;
  className?: string;
}

/** Colorblind-safe status pill: always carries a text label, not just color. */
export function StateBadge({ tone, label, className }: Props) {
  return (
    <span data-tone={tone} className={cn("state-badge", className)}>
      {label}
    </span>
  );
}
