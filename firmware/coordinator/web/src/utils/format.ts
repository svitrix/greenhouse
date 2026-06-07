import type { Quantity } from '../api/types';

export function formatReading(value: number, q: Quantity): string {
  switch (q) {
    case 'temp_c':
    case 'soil_temp_c':   return `${value.toFixed(1)} °C`;
    case 'humidity_pct':
    case 'moisture_pct':
    case 'pct':           return `${Math.round(value)} %`;
    case 'voltage_v':     return `${value.toFixed(2)} V`;
  }
}
