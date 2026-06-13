import {
  Droplets,
  Sprout,
  Thermometer,
  ThermometerSun,
  type LucideIcon,
} from "lucide-react";

/** Sensor-state tones — must match the `data-tone` selectors in index.css. */
export type StateTone =
  | "cold"
  | "cool"
  | "optimal"
  | "moist"
  | "warm"
  | "humid"
  | "wet"
  | "saturated"
  | "hot"
  | "dry"
  | "alert"
  | "offline"
  | "nodata";

export interface SensorState {
  label: string;
  tone: StateTone;
}

/* Band thresholds ported verbatim from device_ui/app.js so the hub stays
   consistent with what the firmware reports. */

export function airTempState(v: number): SensorState {
  if (v < 10) return { label: "cold", tone: "cold" };
  if (v < 18) return { label: "cool", tone: "cool" };
  if (v <= 28) return { label: "optimal", tone: "optimal" };
  if (v <= 32) return { label: "warm", tone: "warm" };
  return { label: "hot", tone: "hot" };
}

export function airHumidityState(v: number): SensorState {
  if (v < 40) return { label: "dry", tone: "dry" };
  if (v <= 75) return { label: "optimal", tone: "optimal" };
  if (v <= 90) return { label: "humid", tone: "humid" };
  return { label: "saturated", tone: "saturated" };
}

export function soilMoistureState(v: number): SensorState {
  if (v < 30) return { label: "dry · water me", tone: "dry" };
  if (v <= 70) return { label: "moist", tone: "moist" };
  if (v <= 90) return { label: "wet", tone: "wet" };
  return { label: "saturated", tone: "saturated" };
}

export function soilTempState(v: number): SensorState {
  if (v < 10) return { label: "cold", tone: "cold" };
  if (v <= 25) return { label: "optimal", tone: "optimal" };
  if (v <= 30) return { label: "warm", tone: "warm" };
  return { label: "hot", tone: "hot" };
}

/** Reading kinds the dashboard renders as primary tiles. */
export type TileKind = "air_temp" | "air_humidity" | "soil_moist" | "soil_temp";

export interface TileSpec {
  kind: TileKind;
  label: string;
  unit: string;
  hint: string;
  icon: LucideIcon;
  decimals: number;
  /** CSS color used for the sparkline + icon accent. */
  accent: string;
  /** Optional reference line drawn on the sparkline (data-space value). */
  markLine?: number;
  stateFn: (v: number) => SensorState;
}

export const TILE_SPECS: TileSpec[] = [
  {
    kind: "air_temp",
    label: "Air · temperature",
    unit: "°C",
    hint: "18–28 °C",
    icon: Thermometer,
    decimals: 1,
    accent: "hsl(var(--k-temp))",
    stateFn: airTempState,
  },
  {
    kind: "air_humidity",
    label: "Air · humidity",
    unit: "%",
    hint: "40–75 % RH",
    icon: Droplets,
    decimals: 0,
    accent: "hsl(var(--k-hum))",
    stateFn: airHumidityState,
  },
  {
    kind: "soil_moist",
    label: "Soil · moisture",
    unit: "%",
    hint: "",
    icon: Sprout,
    decimals: 0,
    accent: "hsl(var(--k-soil))",
    markLine: 30,
    stateFn: soilMoistureState,
  },
  {
    kind: "soil_temp",
    label: "Soil · temperature",
    unit: "°C",
    hint: "roots 15–25 °C",
    icon: ThermometerSun,
    decimals: 1,
    accent: "hsl(var(--k-soil))",
    stateFn: soilTempState,
  },
];

/** UI flags a sensor stale/offline once its last reading is older than this. */
export const STALE_THRESHOLD_S = 300;

export function secondsSince(iso: string | null | undefined, nowMs: number): number | null {
  if (!iso) return null;
  const t = Date.parse(iso);
  if (Number.isNaN(t)) return null;
  return Math.max(0, Math.floor((nowMs - t) / 1000));
}
