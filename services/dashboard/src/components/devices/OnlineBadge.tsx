import { Badge } from "@/components/ui/badge";

export function OnlineBadge({ online }: { online: boolean }) {
  return (
    <Badge
      variant={online ? "default" : "secondary"}
      className={online ? "bg-emerald-500 text-white hover:bg-emerald-500" : ""}
    >
      {online ? "online" : "offline"}
    </Badge>
  );
}
