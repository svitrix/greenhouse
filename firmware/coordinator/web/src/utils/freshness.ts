export type Freshness = 'fresh' | 'recent' | 'stale';

export function freshnessFor(age_s: number): Freshness {
  if (age_s <= 30)  return 'fresh';
  if (age_s <= 120) return 'recent';
  return 'stale';
}

export function formatAge(age_s: number): string {
  if (age_s < 60) return `${age_s}s`;
  if (age_s < 3600) {
    const m = Math.floor(age_s / 60);
    const s = age_s - m * 60;
    return s ? `${m}m ${s}s` : `${m}m`;
  }
  const h = Math.floor(age_s / 3600);
  const m = Math.floor((age_s - h * 3600) / 60);
  return m ? `${h}h ${m}m` : `${h}h`;
}
