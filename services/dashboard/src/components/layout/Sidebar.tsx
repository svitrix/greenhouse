import { NavLink } from "react-router-dom";
import {
  LayoutDashboard,
  Cpu,
  MapPin,
  Sprout,
  Activity,
  KeyRound,
  ShieldCheck,
  Leaf,
} from "lucide-react";
import { cn } from "@/lib/utils";

const NAV = [
  { to: "/", label: "Dashboard", icon: LayoutDashboard, enabled: true },
  { to: "/devices", label: "Devices", icon: Cpu, enabled: true },
  { to: "/locations", label: "Locations", icon: MapPin, enabled: false },
  { to: "/plant-groups", label: "Plant groups", icon: Sprout, enabled: false },
  { to: "/sensors", label: "Sensors", icon: Activity, enabled: false },
  { to: "/pairing", label: "Pairing", icon: KeyRound, enabled: false },
  { to: "/admin-tokens", label: "Admin tokens", icon: ShieldCheck, enabled: false },
];

export function Sidebar() {
  return (
    <aside className="glass m-3 mr-0 flex w-60 flex-col rounded-lg p-4">
      <div className="mb-6 flex items-center gap-2.5 px-1">
        <span className="inline-flex h-9 w-9 items-center justify-center rounded-[12px] bg-grad-ai text-[hsl(var(--lime-foreground))] shadow-[inset_0_1px_0_rgba(255,255,255,0.4),0_6px_18px_-6px_rgba(110,180,60,0.55)]">
          <Leaf className="h-5 w-5" strokeWidth={2} />
        </span>
        <span className="font-display text-h2 font-semibold tracking-tight">Greenhouse</span>
      </div>
      <nav className="space-y-1">
        {NAV.map((item) => {
          const Icon = item.icon;
          if (!item.enabled) {
            return (
              <div
                key={item.to}
                title="Coming soon"
                className="flex cursor-not-allowed items-center gap-2.5 rounded-md px-3 py-2 text-small text-muted-foreground/50"
              >
                <Icon className="h-4 w-4" />
                {item.label}
              </div>
            );
          }
          return (
            <NavLink
              key={item.to}
              to={item.to}
              end={item.to === "/"}
              className={({ isActive }) =>
                cn(
                  "flex items-center gap-2.5 rounded-md px-3 py-2 text-small font-medium transition-colors",
                  isActive
                    ? "bg-primary text-primary-foreground shadow-sm"
                    : "text-muted-foreground hover:bg-accent hover:text-accent-foreground",
                )
              }
            >
              <Icon className="h-4 w-4" />
              {item.label}
            </NavLink>
          );
        })}
      </nav>
    </aside>
  );
}
