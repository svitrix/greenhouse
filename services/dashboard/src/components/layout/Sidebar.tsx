import { NavLink } from "react-router-dom";
import {
  LayoutDashboard,
  Cpu,
  MapPin,
  Sprout,
  Activity,
  KeyRound,
  ShieldCheck,
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
    <aside className="w-64 border-r bg-muted/30 p-4">
      <div className="mb-6 text-lg font-semibold">🌱 Greenhouse</div>
      <nav className="space-y-1">
        {NAV.map((item) => {
          const Icon = item.icon;
          if (!item.enabled) {
            return (
              <div
                key={item.to}
                title="Coming in D-3b/D-3c"
                className="flex cursor-not-allowed items-center gap-2 rounded-md px-3 py-2 text-sm text-muted-foreground/60"
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
                  "flex items-center gap-2 rounded-md px-3 py-2 text-sm",
                  isActive
                    ? "bg-primary text-primary-foreground"
                    : "hover:bg-accent hover:text-accent-foreground",
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
