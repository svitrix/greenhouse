import { Link } from "react-router-dom";
import { ChevronLeft, Leaf, Settings } from "lucide-react";
import { cn } from "@/lib/utils";

interface Props {
  name: string;
  subtitle: string;
  online: boolean;
  onOpenSettings: () => void;
}

export function DeviceHeader({ name, subtitle, online, onOpenSettings }: Props) {
  return (
    <header className="flex flex-wrap items-center gap-2 gap-x-3 pb-4">
      <Link
        to="/devices"
        aria-label="Back to devices"
        className="glass-2 inline-flex h-9 w-9 items-center justify-center rounded-[12px] text-muted-foreground transition-colors hover:text-foreground"
      >
        <ChevronLeft className="h-5 w-5" />
      </Link>
      <span className="inline-flex h-9 w-9 items-center justify-center rounded-[12px] bg-grad-ai text-[hsl(var(--lime-foreground))] shadow-[inset_0_1px_0_rgba(255,255,255,0.4),0_6px_18px_-6px_rgba(110,180,60,0.55)]">
        <Leaf className="h-5 w-5" strokeWidth={2} />
      </span>
      <div className="flex min-w-0 flex-1 flex-col gap-0.5 overflow-hidden">
        <span className="truncate font-display text-h1 font-semibold">{name}</span>
        <span className="num truncate text-tiny uppercase tracking-[0.08em] text-muted-foreground">
          {subtitle}
        </span>
      </div>
      <span
        className={cn(
          "glass-2 inline-flex shrink-0 items-center gap-1.5 rounded-pill px-2.5 py-1.5 text-tiny",
          online ? "text-muted-foreground" : "!border-transparent",
        )}
        style={
          online
            ? undefined
            : { background: "hsl(var(--danger-bg))", color: "hsl(var(--danger-fg))" }
        }
      >
        <span
          className="h-1.5 w-1.5 rounded-full"
          style={{ background: online ? "hsl(var(--primary))" : "hsl(var(--danger))" }}
        />
        {online ? "online" : "offline"}
      </span>
      <button
        onClick={onOpenSettings}
        aria-label="Settings"
        className="glass-2 inline-flex h-10 w-10 shrink-0 items-center justify-center rounded-[12px] text-muted-foreground transition-colors hover:text-foreground"
      >
        <Settings className="h-5 w-5" />
      </button>
    </header>
  );
}
