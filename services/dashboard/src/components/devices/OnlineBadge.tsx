import { StateBadge } from "@/components/device/StateBadge";

export function OnlineBadge({ online }: { online: boolean }) {
  return online ? (
    <StateBadge tone="optimal" label="online" />
  ) : (
    <StateBadge tone="offline" label="offline" />
  );
}
