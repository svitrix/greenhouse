import * as React from "react";
import { cn } from "@/lib/utils";

/** Frosted-glass surface used across the device dashboard. */
const GlassCard = React.forwardRef<
  HTMLDivElement,
  React.HTMLAttributes<HTMLDivElement>
>(({ className, ...props }, ref) => (
  <div
    ref={ref}
    className={cn(
      "glass relative overflow-hidden rounded-md p-4 text-card-foreground",
      className,
    )}
    {...props}
  />
));
GlassCard.displayName = "GlassCard";

export { GlassCard };
