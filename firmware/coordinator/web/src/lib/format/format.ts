export function uptime(s: number): string {
  const d = Math.floor(s / 86400);
  const h = Math.floor((s % 86400) / 3600);
  const m = Math.floor((s % 3600) / 60);
  if (d) return `${d}d ${h}h`;
  if (h) return `${h}h ${m}m`;
  return `${m}m`;
}

export function ago(ms: number | null, now: number = Date.now()): string {
  if (ms == null) return 'never';
  const s = Math.floor((now - ms) / 1000);
  if (s < 60)    return `${s}s ago`;
  if (s < 3600)  return `${Math.floor(s / 60)}m ago`;
  if (s < 86400) return `${Math.floor(s / 3600)}h ago`;
  return `${Math.floor(s / 86400)}d ago`;
}

export function rssi(dbm: number): 'strong' | 'good' | 'fair' | 'weak' {
  if (dbm > -50) return 'strong';
  if (dbm > -65) return 'good';
  if (dbm > -75) return 'fair';
  return 'weak';
}
